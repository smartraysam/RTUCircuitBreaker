#include <ArduinoJson.h>
#include <avr/wdt.h>
#include "SoftwareSerial.h"
#include "Flash_Data.h"
#define RX  11
#define TX  10


SoftwareSerial    GSM(RX, TX); //RX, TX

byte state = LOW;
bool isDebug = true;
//#define HUAWEI
int smscount = 0;
int simAPN = 4;
int netCheck = 0;


#define NETLED  13


static uint32_t timer; //HTTPACTION timers
unsigned long SoftRead;
const long interval = 2000;
unsigned long LEDtimer;

String Data;
String networkAPN;
String pry = "";
String ser ;
String retVal = "";
String gpsResponse;
String  latitude = "6.59354";
String longitude = "3.34576";



#define DEBUG true
String gpsState, timegps = "0";
String data[5];

String response;

int loopGSP = 0;
int address = 0;
boolean isLocateSet = false;
byte value;
char c;

char buffer[85];

char server[] = "susejgroup.cc"; // "";//also change the Host line in httpRequest()  //.. configured in SGC Studio "www.susejtest.us"; //
char path[] =  "/api/getfeedback?";

bool errorState = true;


#define relay_red_phase1 6
#define relay_red_phase2 A15

#define relay_blue_phase1  9
#define relay_blue_phase2  7

#define Gndrelay  51


#define led1l  31
#define led1g  32
#define led2l  33
#define led2g  34

int f_one, f_two, f_three = 0;
StaticJsonDocument<200> doc;
DeserializationError error;

void setup()
{
  // Init serial communication
  Serial.begin(115200);
  Serial.println("System Startup");
  delay(2000);
  GSM.begin(9600);

  pinMode(NETLED,  OUTPUT);
  pinMode(relay_red_phase1,  OUTPUT);
  pinMode(relay_red_phase2,  OUTPUT);
  pinMode(relay_blue_phase1 ,  OUTPUT);
  pinMode(relay_blue_phase2 , OUTPUT);

  pinMode(Gndrelay,  OUTPUT);
  pinMode(led1l,  OUTPUT);
  pinMode(led1g ,  OUTPUT);
  pinMode(led2l , OUTPUT);
  pinMode(led2g , OUTPUT);
  digitalWrite(Gndrelay, 0);
  digitalWrite(led1g, 0);
  digitalWrite(led2g, 0);

  wdt_disable();
  wdt_enable(WDTO_8S);// 4 sec

  strcpy_P(buffer, (char*)pgm_read_word(&(comm_Data[0])));
  ser = (String)buffer;
  Debug("Device Serial Number: " + ser + "\n");
  while (1) {
    //Print "AT" to check if GSM is ON
    wdt_reset();
    GSM.println(F("AT"));
    delay(100);
    if (GSM.available()) {
      pry = GSM.readString();
    }
    //If COntroller receives "OK", jump out of loop
    if (pry.indexOf("OK") != -1) break;
  }
  pry = "";
  Debug("Setting GSM HTTP connection:\n");
  setUpGSM();
  delay(500); //wait for stability
  while (simAPN == 4 && netCheck < 4) {
    wdt_reset();
    attachInternet();
    netCheck++;
    wdt_reset();
    delay(1500);
    wdt_reset();
  }
  Debug(networkAPN);
  wdt_reset();
  delay(500);
  setGSMHTTP(networkAPN);
  Debug(F("\nGSM Setup Complete\n"));
  delay(500);
  sendData("AT+CMGF=1", 1000);
  delay(50);
}
void loop()
{
  response = readSms();
  error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  } else {
    controlRelay(doc);
  }
  f_one = digitalRead(relay_red_phase1);
  f_two = digitalRead(relay_blue_phase1);
  blinkNETLED();
  wdt_reset();
  String address, url;
  unsigned long StateMillis = millis();
  address = "AT+HTTPPARA=\"URL\",\"http://" + String(server) + String(path);
  address.concat("serial_number=" + String(ser) + "&f1=" + String(f_one) + "&f2=" + String(f_two) + "&f3=" + String(1));
  address.concat("\"");
  address.replace(" ", "");
  wdt_reset();
  response = SendData(address); //A function to send GPS coordinates to server
  Debug(response + "\n");
  response = response.substring(response.indexOf(F("{")), response.indexOf(F("}")) + 1);
  error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  controlRelay(doc);
  GSM.flush();
  Serial.flush();
}
void controlRelay(StaticJsonDocument<200> doc) {
  const char* c1 = doc["c1"]; // "1"
  const char* c2 = doc["c2"]; // "0"
  const char* c3 = doc["c3"]; // "1"
  Serial.println(String(c1));
  Serial.println(String(c2));
  if (String(c1) == "1") {
    Serial.println(F("c1 on"));
    digitalWrite(relay_red_phase1,  1);
    digitalWrite(relay_red_phase2,  0);
    digitalWrite(led1l,  1);
    f_one = 1;
  } else if (String(c1) == "0") {
    Serial.println(F("c1 off"));
    digitalWrite(relay_red_phase1,  0);
    digitalWrite(relay_red_phase2,  1);
    digitalWrite(led1l,  0);
    f_one = 0;
  }
  if (String(c2) == "1") {
    Serial.println(F("c2 on"));
    digitalWrite(relay_blue_phase1 ,  1);
    digitalWrite(relay_blue_phase2 , 0);
    digitalWrite(led2l,  1);
    f_two = 1;
  }  else if (String(c2) == "0") {
    Serial.println(F("c2 off"));
    digitalWrite(relay_blue_phase1 , 0);
    digitalWrite(relay_blue_phase2 , 1);
    digitalWrite(led2l,  0);
    f_two = 0;
  }
}
