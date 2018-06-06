//  To upload through terminal you can use: curl -u admin:admin -F "image=@firmware.bin" esp8266-webupdate.local/firmware

/*
pin connections on breadboard
d3 = dh22
a0 = button
*/

//#define UDP

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include "DHT.h"

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal

#ifdef UDP
#include <WiFiUdp.h>
#endif


#define DHTPIN 0

#define ON LOW
#define OFF HIGH

#define DHTTYPE DHT22
#define DHTREADFREQUENCY 30000    // read once every 30 sec
#define DHTFAILFREQUENCY  3000    // if read failes try again after 3 seconds


#define BUTTON1LOW   1024
#define BUTTON1HIGH  1024

#define BUTTON2LOW   950
#define BUTTON2HIGH  965

#define BUTTON3LOW   835
#define BUTTON3HIGH  855

#define BUTTON4LOW   655
#define BUTTON4HIGH  685

#define BUTTON5LOW   500
#define BUTTON5HIGH  535

#define BUTTON6LOW   465
#define BUTTON6HIGH  485

#define BUTTON7LOW   350
#define BUTTON7HIGH  380

#define BUTTON8LOW   145
#define BUTTON8HIGH  160

#define BUTTON1 1
#define BUTTON2 2
#define BUTTON3 3
#define BUTTON4 4
#define BUTTON5 5
#define BUTTON6 6
#define BUTTON7 7
#define BUTTON8 8


struct relay {
  const char name[50];
  const int pin;
  bool status;
  unsigned long lastSwitch;
};

relay relay1 { "relay1", D8, OFF, 0 };
relay relay2 { "relay2", D7, OFF, 0 };
relay relay3 { "relay3", D6, OFF, 0 };
relay relay4 { "relay4", D5, OFF, 0 };
relay relay5 { "relay5", D0, OFF, 0 };
relay relay6 { "relay6", D4, OFF, 0 };
relay relay7 { "relay7", D2, OFF, 0 };
relay relay8 { "relay8", D1, OFF, 0 };



// Button debounce and ADC converting variables
int relaytimer = 500;
int reading;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int tmpButtonState = LOW;    // the current reading from the input pin
unsigned long lastButtonRead = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
int debounceDelay = 50;    // the debounce time; increase if the output flickers
int analogReadDelay = 20;


// DHT variabels
float humidity, temp, hi, dew;
unsigned long timerdht = 0;


// Wifi/Webserver variables
const char* host = "garage-back";
const char* update_path = "/firmware";
const char* update_username = "toby";
const char* update_password = "toby";










DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiManager wifiManager;

#ifdef UDP
WiFiUDP Udp;
#endif





void printConstants() {
  Serial.println("garage-control");
  Serial.print("Sensor Polling interval: ");
  Serial.print(DHTREADFREQUENCY);
  Serial.print("\tDHT Sensor: ");
  Serial.print(DHTTYPE);
  Serial.print("\tPin: ");
  Serial.println(DHTPIN);
}


void printDhtSerial() {
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" *C\t");
  Serial.print("DewPoint: ");
  Serial.print(dew);
  Serial.print(" *C\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *C");
}


float dewPoint(float celsius, float humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T);
}




// Handle relay calls .... 
void buttonRelay(relay *relayPtr) {
  if ( (millis() - (*relayPtr).lastSwitch) > relaytimer ) {
    (*relayPtr).lastSwitch = millis();
    (*relayPtr).status = !(*relayPtr).status;
    digitalWrite((*relayPtr).pin, (*relayPtr).status);
  }
}

void httpRelayStatus(relay *relayPtr) {
   bool status;
   char buffer[50];
   status = digitalRead((*relayPtr).pin);
   sprintf(buffer, "%s %s\n", (*relayPtr).name, (*relayPtr).status?"off":"on"); 
   httpServer.send(200, "text/plain", buffer); 
}

void httpRelaySet(relay *relayPtr) {
   char buffer[50];
   sprintf(buffer, "%s %s\n", (*relayPtr).name, (*relayPtr).status?"off":"on");
   digitalWrite((*relayPtr).pin, (*relayPtr).status);
   httpServer.send(200, "text/plain", buffer);
}


void httpRelay1SetOn()     {   relay1.status = ON;                    httpRelaySet(&relay1);    }
void httpRelay1SetOff()    {   relay1.status = OFF;                   httpRelaySet(&relay1);    }
void httpRelay1SetToggle() {   relay1.status = !relay1.status;        httpRelaySet(&relay1);    }
void httpRelay1Status()    {                                          httpRelayStatus(&relay1); }

void httpRelay2SetOn()     {   relay2.status = ON;                    httpRelaySet(&relay2);    }
void httpRelay2SetOff()    {   relay2.status = OFF;                   httpRelaySet(&relay2);    }
void httpRelay2SetToggle() {   relay2.status = !relay2.status;        httpRelaySet(&relay2);    }
void httpRelay2Status()    {                                          httpRelayStatus(&relay2); }

void httpRelay3SetOn()     {   relay3.status = ON;                    httpRelaySet(&relay3);    }
void httpRelay3SetOff()    {   relay3.status = OFF;                   httpRelaySet(&relay3);    }
void httpRelay3SetToggle() {   relay3.status = !relay3.status;        httpRelaySet(&relay3);    }
void httpRelay3Status()    {                                          httpRelayStatus(&relay3); }

void httpRelay4SetOn()     {   relay4.status = ON;                    httpRelaySet(&relay4);    }
void httpRelay4SetOff()    {   relay4.status = OFF;                   httpRelaySet(&relay4);    }
void httpRelay4SetToggle() {   relay4.status = !relay4.status;        httpRelaySet(&relay4);    }
void httpRelay4Status()    {                                          httpRelayStatus(&relay4); }

void httpRelay5SetOn()     {   relay5.status = ON;                    httpRelaySet(&relay5);    }
void httpRelay5SetOff()    {   relay5.status = OFF;                   httpRelaySet(&relay5);    }
void httpRelay5SetToggle() {   relay5.status = !relay5.status;        httpRelaySet(&relay5);    }
void httpRelay5Status()    {                                          httpRelayStatus(&relay5); }

void httpRelay6SetOn()     {   relay6.status = ON;                    httpRelaySet(&relay6);    }
void httpRelay6SetOff()    {   relay6.status = OFF;                   httpRelaySet(&relay6);    }
void httpRelay6SetToggle() {   relay6.status = !relay6.status;        httpRelaySet(&relay6);    }
void httpRelay6Status()    {                                          httpRelayStatus(&relay6); }

void httpRelay7SetOn()     {   relay7.status = ON;                    httpRelaySet(&relay7);    }
void httpRelay7SetOff()    {   relay7.status = OFF;                   httpRelaySet(&relay7);    }
void httpRelay7SetToggle() {   relay7.status = !relay7.status;        httpRelaySet(&relay7);    }
void httpRelay7Status()    {                                          httpRelayStatus(&relay7); }

void httpRelay8SetOn()     {   relay8.status = ON;                    httpRelaySet(&relay8);    }
void httpRelay8SetOff()    {   relay8.status = OFF;                   httpRelaySet(&relay8);    }
void httpRelay8SetToggle() {   relay8.status = !relay8.status;        httpRelaySet(&relay8);    }
void httpRelay8Status()    {                                          httpRelayStatus(&relay8); }


void httpRelay() {

  httpServer.on("/relay1/on",      httpRelay1SetOn);
  httpServer.on("/relay1/off",     httpRelay1SetOff);
  httpServer.on("/relay1/toggle",  httpRelay1SetToggle);
  httpServer.on("/relay1/status",  httpRelay1Status);

  httpServer.on("/relay2/on",      httpRelay2SetOn);
  httpServer.on("/relay2/off",     httpRelay2SetOff);
  httpServer.on("/relay2/toggle",  httpRelay2SetToggle);
  httpServer.on("/relay2/status",  httpRelay2Status);

  httpServer.on("/relay3/on",      httpRelay3SetOn);
  httpServer.on("/relay3/off",     httpRelay3SetOff);
  httpServer.on("/relay3/toggle",  httpRelay3SetToggle);
  httpServer.on("/relay3/status",  httpRelay3Status);

  httpServer.on("/relay4/on",      httpRelay4SetOn);
  httpServer.on("/relay4/off",     httpRelay4SetOff);
  httpServer.on("/relay4/toggle",  httpRelay4SetToggle);
  httpServer.on("/relay4/status",  httpRelay4Status);

  httpServer.on("/relay5/on",      httpRelay5SetOn);
  httpServer.on("/relay5/off",     httpRelay5SetOff);
  httpServer.on("/relay5/toggle",  httpRelay5SetToggle);
  httpServer.on("/relay5/status",  httpRelay5Status);

  httpServer.on("/relay6/on",      httpRelay6SetOn);
  httpServer.on("/relay6/off",     httpRelay6SetOff);
  httpServer.on("/relay6/toggle",  httpRelay6SetToggle);
  httpServer.on("/relay6/status",  httpRelay6Status);

  httpServer.on("/relay7/on",      httpRelay7SetOn);
  httpServer.on("/relay7/off",     httpRelay7SetOff);
  httpServer.on("/relay7/toggle",  httpRelay7SetToggle);
  httpServer.on("/relay7/status",  httpRelay7Status);

  httpServer.on("/relay8/on",      httpRelay8SetOn);
  httpServer.on("/relay8/off",     httpRelay8SetOff);
  httpServer.on("/relay8/toggle",  httpRelay8SetToggle);
  httpServer.on("/relay8/status",  httpRelay8Status);

}





void setup(void){

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");

  dht.begin();
  printConstants();

  digitalWrite(relay1.pin, relay1.status);
  digitalWrite(relay2.pin, relay2.status);
  digitalWrite(relay3.pin, relay3.status);
  digitalWrite(relay4.pin, relay4.status);
  digitalWrite(relay5.pin, relay5.status);
  digitalWrite(relay6.pin, relay6.status);
  digitalWrite(relay7.pin, relay7.status);
  digitalWrite(relay8.pin, relay8.status);

  pinMode(relay1.pin, OUTPUT);
  pinMode(relay2.pin, OUTPUT);
  pinMode(relay3.pin, OUTPUT);
  pinMode(relay4.pin, OUTPUT);
  pinMode(relay5.pin, OUTPUT);
  pinMode(relay6.pin, OUTPUT);
  pinMode(relay7.pin, OUTPUT);
  pinMode(relay8.pin, OUTPUT);

  Serial.println("init WiFi...");
  wifiManager.autoConnect();


  Serial.println("init Webserver...");
  // handle humidity and sensor readings
  httpServer.on("/humidity",      [](){
                                        Serial.println("HTTP read Humidity");
                                        char buffer[10];
                                        sprintf(buffer, "%f", humidity);
                                        httpServer.send(200, "text/plain", buffer);
                                     });
  httpServer.on("/temp",          [](){
                                        Serial.println("HTTP read Temperature");
                                        char buffer[10];
                                        sprintf(buffer, "%f", temp);
                                        httpServer.send(200, "text/plain", buffer);
                                     });
  httpServer.on("/dew",           [](){
                                        Serial.println("HTTP read Dewpoint");
                                        char buffer[10];
                                        sprintf(buffer, "%f", dew);
                                        httpServer.send(200, "text/plain", buffer);
                                     });
  httpServer.on("/hi",            [](){
                                        Serial.println("HTTP read Heat Index");
                                        char buffer[10];
                                        sprintf(buffer, "%f", hi);
                                        httpServer.send(200, "text/plain", buffer);
                                     });

  // Handle Relays
  httpRelay();


  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  Serial.println("start Webserver");
  httpServer.begin();

  Serial.println("init DNS");
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
  Serial.println();
}


void loop(void){
  httpServer.handleClient();


  if ( millis() > timerdht ) {    // switch fan not more than once every FANSWITCHFREQUENCY
    Serial.print("DHT ... ");

    //// Read sensor
    humidity = dht.readHumidity();
    temp = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity)) {
      Serial.println("Failed to read sensor!");
      timerdht = millis() + DHTFAILFREQUENCY;    // if sensor reading failed try again after FAILFREQUENCY
    } else {
      Serial.println("OK");
      timerdht = millis() + DHTREADFREQUENCY;

      // Adjust values by constant
      //humidity += humidityOffset;

      // Compute heat index and dewpoint
      dew = dewPoint(temp, humidity);
      hi = dht.computeHeatIndex(temp, humidity, false);

      printDhtSerial();   //// printing it all on the Serial if we actually got values
    }

  }



  if ((millis() - lastButtonRead) > analogReadDelay ) {
    lastButtonRead = millis();

    reading = analogRead(A0);
    //Serial.println(reading);

    if      (reading>=BUTTON1LOW && reading<=BUTTON1HIGH) tmpButtonState = BUTTON1;       //Read switch 1
    else if (reading>=BUTTON2LOW && reading<=BUTTON2HIGH) tmpButtonState = BUTTON2;       //Read switch 2
    else if (reading>=BUTTON3LOW && reading<=BUTTON3HIGH) tmpButtonState = BUTTON3;       //Read switch 3
    else if (reading>=BUTTON4LOW && reading<=BUTTON4HIGH) tmpButtonState = BUTTON4;       //Read switch 4
    else if (reading>=BUTTON5LOW && reading<=BUTTON5HIGH) tmpButtonState = BUTTON5;       //Read switch 5
    else if (reading>=BUTTON6LOW && reading<=BUTTON6HIGH) tmpButtonState = BUTTON6;       //Read switch 6
    else if (reading>=BUTTON7LOW && reading<=BUTTON7HIGH) tmpButtonState = BUTTON7;       //Read switch 7
    else if (reading>=BUTTON8LOW && reading<=BUTTON8HIGH) tmpButtonState = BUTTON8;       //Read switch 8
    else    tmpButtonState = LOW;                                                         //No button is pressed;
  
    if (tmpButtonState != lastButtonState) {
      lastDebounceTime = millis();
    }
  
    if ((millis() - lastDebounceTime) > debounceDelay) {
      buttonState = tmpButtonState;
    }
    lastButtonState = tmpButtonState;
 
 
    switch(buttonState){
      case LOW:
        break;
      case BUTTON1:
        buttonRelay(&relay1);
        break;
      case BUTTON2:
        buttonRelay(&relay2);
        break;
      case BUTTON3:
        buttonRelay(&relay3);
        break;
      case BUTTON4:
        buttonRelay(&relay4);
        break;
      case BUTTON5:
        buttonRelay(&relay5);
        break;
      case BUTTON6:
        buttonRelay(&relay6);
        break;
      case BUTTON7:
        buttonRelay(&relay7);
        break;
      case BUTTON8:
        buttonRelay(&relay8);
        break;
    }

#ifdef UDP
    if ( reading > 100 ) {
      char buffer[100];
      sprintf(buffer, "reading %d\tbutton: %d\n", reading, buttonState);
      Udp.beginPacket("192.168.10.111", 8080);
      Udp.write(buffer);
      Udp.endPacket();
    }
#endif

  }
}
