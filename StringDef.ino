void setUpGSM() {
  String response;
  while (true) {
    wdt_reset();
    response = "";
    GSM.println(F("AT"));
    response = GSM_Data();
    if (response.indexOf(F("OK")) != -1) {
      wdt_reset();
      Debug(F("GSM Found!\n"));
      response = "";
      GSM.println(F("AT+CPIN?"));
      delay(1000);
      response = GSM_Data();
      Debug(response + "\n");
      if (response.indexOf(F("READY")) != -1) {
        GSM.println(F("AT+CSQ"));
        GSM_Data();
        GSM.println(F("AT+CREG?"));
        GSM_Data();
        errorState = false;
        break;
      }
    }
  }
}//End void setUpGSM()
void setGSMHTTP(String networkAPN) {
  wdt_reset();
  GSM.println("AT+CGATT?"); //Attach or Detach from GPRS Support
  delay(200);
  GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  delay(200);
  GSM.println("AT+SAPBR=3,1,\"APN\"," + networkAPN); //setting the APN, Access point name string
  delay(200);
  wdt_reset();
  GSM.println("AT+SAPBR=1,1");//setting the SAPBR
  delay(200);
  GSM.println("AT+CSQ"); // Signal quality check
  delay(500);
  GSM.println("AT+HTTPTERM"); //init the HTTP request
  delay(500);
  GSM.println("AT+HTTPINIT"); //init the HTTP request
}

int IDSIM() {
  String response = "";
  unsigned int net = 4;
  Debug(F("Checking SIM Network!\n"));
  if (!errorState) {
    errorState = true;
    while (true) {
      wdt_reset();
      GSM.println(F("AT+COPS?"));
      response = GSM_Data();
      Debug(response);
      if (response.indexOf("Glo") != -1)
        net = 0;
      else if (response.indexOf("MTN") != -1)
        net = 1;
      else if (response.indexOf("Airtel") != -1 || response.indexOf("Econet") != -1)
        net = 2;
      else if (response.indexOf("9mobile") != -1 || response.indexOf("62160") != -1)
        net = 3;
      else {
        net = 4;
        Debug(F("Network Not defined:  Using default APN\n"));
      }
      if (net <= 4) {
        errorState = false;
        break;
      }

    }
  } else {
    Debug(F("Error state is active!\n"));
  }
  return net;
}//End int IDSIM()

String GSM_Data() {
  String _data;
  unsigned long GSM_data_timeout = millis();
  while (GSM.available() == 0)
    if (millis() > (GSM_data_timeout + 1000)) break;
  delay(50);
  while (GSM.available()) {
    char c = GSM.read();
    _data += c;
  }
  return _data;
}//End String GSM_Data()

bool attachInternet() {
  wdt_reset();
  String response;
  bool state = false;
  unsigned int simID = IDSIM();
  Debug("\n" + (String)simID);
  simAPN = simID;
  if (simID <= 4) {
    char buffer[75];
    strcpy_P(buffer, (char*)pgm_read_word(&(APN[simID])));
    Debug(F("APN: "));
    Debug((String)buffer);
    Debug(F("\n"));
    Debug(F("simID: "));
    Debug((String)simID);
    Debug(F("\n"));
    networkAPN = (String)buffer;
    GSM.println(F("AT+CGATT?"));
    GSM_Data();
    GSM.println(F("AT+CSTT?"));
    response = GSM_Data();
    if (response.indexOf(buffer) < 0) {
      String apn = F("AT+CSTT=\"");
      apn += (String)buffer;
      apn += F("\"");
      apn += F(",\"\",\"\"");
      GSM.println(apn);
    }
    GSM_Data();
    GSM.println(F("AT+CIICR"));
    GSM_Data();
    GSM.println(F("AT+CIFSR"));
    response = "";
    response = GSM_Data();
    if (response.indexOf(F("10.")) != -1) {
      wdt_reset();
      GSM_Data();
      GSM.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
      delay(500);
      GSM_Data();
      GSM.println("AT+SAPBR=3,1,\"APN\"," + networkAPN);//setting the APN, Access point name string
      delay(500);
      wdt_reset();
      GSM_Data();
      GSM.println("AT+SAPBR=1,1");//setting the SAPBR
      delay(500);
      GSM_Data();
      state = true;
    }

  } else {
    Debug(F("Unknown Network, Attach Internet failed!\n"));
  }
  return state;
}//End bool attachInternet()

void Debug(String msg) {
  if (isDebug) {
    Serial.print(msg);
  }
}//End Debug()

//Function to send IoT data to server
String SendData(String url) {
  wdt_reset();
  String response;
  Debug(url + "\n");
  Debug(F("opening server request!\n"));
  GSM.println(url);
  delay(200);
  GSM.println("AT+HTTPACTION=0");
  wdt_reset();
  delay(1000);
  GSM_Data();
  wdt_reset();
  GSM.println("AT+HTTPREAD");// read the data from the website you access
  delay(2000);
  wdt_reset();
  response = GSM_Data();
  GSM.println("  ");
  return response;
}

void getGPSData(String command , const int timeout , boolean debug) {
  wdt_reset();
  Serial.println("Getting GPS Info.");
  GSM.println(command);
  String lastestLat;
  String lastestLng;
  gpsState = "";
  timegps  = "";
  long int time = millis();
  int i = 0;
  wdt_reset();
  while ((time + timeout) > millis()) {
    while (GSM.available()) {
      char c = GSM.read();
      if (c != ',') {
        data[i] += c;
        delay(100);
        wdt_reset();
      } else {
        i++;
        wdt_reset();
      }
      if (i == 5) {
        wdt_reset();
        delay(100);
        goto exitL;
      }
    }
} exitL:
  if (debug) {
    gpsState = data[1];
    timegps = data[2];
    lastestLat = data[3];
    lastestLng = data[4];
    wdt_reset();
  }
  Serial.println("State  :" + gpsState);
  Serial.println("Time  :" + timegps);
  Serial.println("latitude  :" + lastestLat);
  Serial.println("Longitude  :" + lastestLng);
  if (gpsState == "1") {
    latitude = lastestLat;
    longitude = lastestLng;
    isLocateSet = true;
  }
}

String sendData(String command, const int timeout)
{
  String response = "";
  GSM.println(command);
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    wdt_reset();
    while (GSM.available())
    {
      char c = GSM.read();
      response += c;
    }
  }
  Debug(response);
  return response;
}
// digital input function to collect number of avaiable channel and used channel


void blinkNETLED() {
  int blinkInterval = 150;
  if (millis() > (LEDtimer + blinkInterval)) {
    LEDtimer = millis();
    digitalWrite(NETLED, !digitalRead(NETLED));
  }
}//End void blinkNETLED()



void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");
}

String readSms() {
  String ctrlText;
  smscount++;
  wdt_reset();
  String response = sendData("AT+CMGR=" + String(smscount), 500); //read message at index i
  if (response.indexOf("+CMGR:") != -1) {
    ctrlText = response.substring(response.indexOf(F("{")), response.indexOf(F("}")) + 1);//extracts text frm JSON in the sms
    Serial.println(ctrlText);
    sendData("AT+CMGD=" + String(smscount), 50);
    wdt_reset();
  }
  smscount = 0;
  return ctrlText;
}
