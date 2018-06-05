//  To upload through terminal you can use: curl -u admin:admin -F "image=@firmware.bin" esp8266-webupdate.local/firmware

/*
pin connections on breadboard
d3 = dh22
a0 = button

relay
d8 = i1 = 15
d7 = i2 = 13
d6 = i3 = 12
d5 = i4 = 14
d0 = i5 = 16
d4 = i6 =  2
d2 = i7 =  4
d1 = i8 =  5
*/

//#define UDP

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include "DHT.h"

#ifdef UDP
#include <WiFiUdp.h>
#endif

#define RELAY1 15
#define RELAY2 13
#define RELAY3 12
#define RELAY4 14
#define RELAY5 16
#define RELAY6 2
#define RELAY7 4
#define RELAY8 5

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


// Button debounce and ADC converting variables
int reading;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int tmpButtonState = LOW;    // the current reading from the input pin
unsigned long lastButtonRead = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
int debounceDelay = 50;    // the debounce time; increase if the output flickers
int analogReadDelay = 20;


int relaytimer = 500;
unsigned long lastRelay1Switch = 0;
unsigned long lastRelay2Switch = 0;
unsigned long lastRelay3Switch = 0;
unsigned long lastRelay4Switch = 0;
unsigned long lastRelay5Switch = 0;
unsigned long lastRelay6Switch = 0;
unsigned long lastRelay7Switch = 0;
unsigned long lastRelay8Switch = 0;

bool relay1status = OFF;
bool relay2status = OFF;
bool relay3status = OFF;
bool relay4status = OFF;
bool relay5status = OFF;
bool relay6status = OFF;
bool relay7status = OFF;
bool relay8status = OFF;



// Wifi/Webserver variables
const char* host = "garage-back";
const char* update_path = "/firmware";
const char* update_username = "toby";
const char* update_password = "toby";
const char* ssid = "lazog";
const char* password = "abcd1234";


// DHT variabels
float humidity, temp, hi, dew;
unsigned long timerdht = 0;








DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

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




// Handle relay HTTP calls .... 

bool httpRelayStatus(int pin, char* name) {
   bool status;
   char buffer[50];
   status = digitalRead(pin);
   sprintf(buffer, "%s %s", name, status?"off":"on"); 
   Serial.println(buffer);
   httpServer.send(200, "text/plain", buffer); 

   return status;
}

bool httpRelaySetOld(int pin, char* name, bool status) {
   char buffer[50];
   sprintf(buffer, "%s %s", name, status?"off":"on");
   Serial.println(buffer);
   digitalWrite(pin, status);
   httpServer.send(200, "text/plain", buffer);

   return status;
}

struct relay {
  int pin;
  bool status;
  char name[50];
};

relay relaytmp;
relay relay1 { RELAY1, OFF, "relay1" };

void httpRelaySet() {
   char buffer[50];
   sprintf(buffer, "%s %s", relaytmp.name, relaytmp.status?"off":"on");
   Serial.println(buffer);
   digitalWrite(relaytmp.pin, relaytmp.status);
   httpServer.send(200, "text/plain", buffer);
}




bool httpRelay(int pin, char* name, bool status) {
  char uriOn[20];
  char uriOff[20];
  char uriToggle[20];
  char uriStatus[20];

  sprintf(uriOn,     "/%s/on",     name);
  sprintf(uriOff,    "/%s/off",    name);
  sprintf(uriToggle, "/%s/toggle", name);
  sprintf(uriStatus, "/%s/status", name);

  relaytmp = relay1;

  httpServer.on(uriOn,      httpRelaySet);
  //httpServer.on(uriOn,      [](){ status = httpRelaySet(pin, name, ON); });
  //httpServer.on(uriOff,     [](){ status = httpRelaySet(pin, name, OFF); });
  //httpServer.on(uriToggle,  [](){ status = httpRelaySet(pin, name, !status); });
  //httpServer.on(uriStatus,  [](){ status = httpRelayStatus(pin, name); });

  return status;
}





void setup(void){

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");

  dht.begin();
  printConstants();


  digitalWrite(RELAY1, relay1status);
  digitalWrite(RELAY2, relay2status);
  digitalWrite(RELAY3, relay3status);
  digitalWrite(RELAY4, relay4status);
  digitalWrite(RELAY5, relay5status);
  digitalWrite(RELAY6, relay6status);
  digitalWrite(RELAY7, relay7status);
  digitalWrite(RELAY8, relay8status);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY7, OUTPUT);
  pinMode(RELAY8, OUTPUT);


  Serial.println("init WiFi...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying...");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

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





  // Handle Relay 1
  relay1status = httpRelay(RELAY1, "relay1", relay1status);

  // Handle Relay 2 .... YES this can be coded better ....
  httpServer.on("/relay2/on", [](){
    Serial.println("HTTP relay2 on");
    digitalWrite(RELAY2, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay2/off", [](){
    Serial.println("HTTP relay2 off");
    digitalWrite(RELAY2, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay2/status", [](){
    Serial.println("HTTP relay2 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY2));
    httpServer.send(200, "text/plain", buffer);
  });


  // Handle Relay 3 .... YES this can be coded better ....
  httpServer.on("/relay3/on", [](){
    Serial.println("HTTP relay3 on");
    digitalWrite(RELAY3, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay3/off", [](){
    Serial.println("HTTP relay3 off");
    digitalWrite(RELAY3, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay3/status", [](){
    Serial.println("HTTP relay3 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY3));
    httpServer.send(200, "text/plain", buffer);
  });


  // Handle Relay 4 .... YES this can be coded better ....
  httpServer.on("/relay4/on", [](){
    Serial.println("HTTP relay4 on");
    digitalWrite(RELAY4, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay4/off", [](){
    Serial.println("HTTP relay4 off");
    digitalWrite(RELAY4, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay4/status", [](){
    Serial.println("HTTP relay4 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY4));
    httpServer.send(200, "text/plain", buffer);
  });



  // Handle Relay 5 .... YES this can be coded better ....
  httpServer.on("/relay5/on", [](){
    Serial.println("HTTP relay5 on");
    digitalWrite(RELAY5, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay5/off", [](){
    Serial.println("HTTP relay5 off");
    digitalWrite(RELAY5, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay5/status", [](){
    Serial.println("HTTP relay5 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY5));
    httpServer.send(200, "text/plain", buffer);
  });



  // Handle Relay 6 .... YES this can be coded better ....
  httpServer.on("/relay6/on", [](){
    Serial.println("HTTP relay6 on");
    digitalWrite(RELAY6, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay6/off", [](){
    Serial.println("HTTP relay6 off");
    digitalWrite(RELAY6, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay6/status", [](){
    Serial.println("HTTP relay6 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY6));
    httpServer.send(200, "text/plain", buffer);
  });



  // Handle Relay 7 .... YES this can be coded better ....
  httpServer.on("/relay7/on", [](){
    Serial.println("HTTP relay7 on");
    digitalWrite(RELAY7, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay7/off", [](){
    Serial.println("HTTP relay7 off");
    digitalWrite(RELAY7, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay7/status", [](){
    Serial.println("HTTP relay7 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY7));
    httpServer.send(200, "text/plain", buffer);
  });



  // Handle Relay 8 .... YES this can be coded better ....
  httpServer.on("/relay8/on", [](){
    Serial.println("HTTP relay8 on");
    digitalWrite(RELAY8, ON);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay8/off", [](){
    Serial.println("HTTP relay8 off");
    digitalWrite(RELAY8, OFF);
    httpServer.send(200, "text/plain", "OK");
  });
  httpServer.on("/relay8/status", [](){
    Serial.println("HTTP relay8 status");
    char buffer[2];
    sprintf(buffer, "%d", digitalRead(RELAY8));
    httpServer.send(200, "text/plain", buffer);
  });


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
        if ( (millis() - lastRelay1Switch) > relaytimer ) {
          lastRelay1Switch = millis();
          relay1status = !relay1status;
          digitalWrite(RELAY1, relay1status);
        }
        break;
      case BUTTON2:
        if ( (millis() - lastRelay2Switch) > relaytimer ) {
          lastRelay2Switch = millis();
          relay2status = !relay2status;
          digitalWrite(RELAY2, relay2status);
        }
        break;
      case BUTTON3:
        if ( (millis() - lastRelay3Switch) > relaytimer ) {
          lastRelay3Switch = millis();
          relay3status = !relay3status;
          digitalWrite(RELAY3, relay3status);
        }
        break;
      case BUTTON4:
        if ( (millis() - lastRelay4Switch) > relaytimer ) {
          lastRelay4Switch = millis();
          relay4status = !relay4status;
          digitalWrite(RELAY4, relay4status);
        }
        break;
      case BUTTON5:
        if ( (millis() - lastRelay5Switch) > relaytimer ) {
          lastRelay5Switch = millis();
          relay5status = !relay5status;
          digitalWrite(RELAY5, relay5status);
        }
        break;
      case BUTTON6:
        if ( (millis() - lastRelay6Switch) > relaytimer ) {
          lastRelay6Switch = millis();
          relay6status = !relay6status;
          digitalWrite(RELAY6, relay6status);
        }
        break;
      case BUTTON7:
        if ( (millis() - lastRelay7Switch) > relaytimer ) {
          lastRelay7Switch = millis();
          relay7status = !relay7status;
          digitalWrite(RELAY7, relay7status);
        }
        break;
      case BUTTON8:
        if ( (millis() - lastRelay8Switch) > relaytimer ) {
          lastRelay8Switch = millis();
          relay8status = !relay8status;
          digitalWrite(RELAY8, relay8status);
        }
        break;
    }

#ifdef UDP
    if ( reading > 100 ) {
      char buffer[100];
      sprintf(buffer, "reading %d\tbutton: %d relay1: %d/%d relay2: %d/%d relay3: %d/%d", reading, buttonState, relay1status, lastRelay1Switch, relay2status, lastRelay2Switch, relay3status, lastRelay3Switch );
      Udp.beginPacket("192.168.10.111", 8080);
      Udp.write(buffer);
      Udp.endPacket();
    }
#endif

  }
}
