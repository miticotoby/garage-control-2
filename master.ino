/*
  To upload through terminal you can use: curl -u admin:admin -F "image=@firmware.bin" esp8266-webupdate.local/firmware
*/

/*

pin connections on breadboard
d0 = dh22
a0 = button

relay
d8 = i1
d7 = i2
d6 = i3
d5 = i4
d4 = i5
d3 = i6
d2 = i7
d1 = i8

*/



#define RELAY1 15
#define RELAY2 13
#define RELAY3 12
#define RELAY4 14
#define RELAY5 2
#define RELAY6 16
#define RELAY7 4
#define RELAY8 5

#define DHTPIN 0


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "DHT.h"

#define OFF LOW
#define ON HIGH

#define DHTTYPE DHT22
#define DHTREADFREQUENCY 30000    // read once every 30 sec
#define DHTFAILFREQUENCY  3000    // if read failes try again after 3 seconds


// Wifi/Webserver variables
const char* host = "garage-back";
const char* update_path = "/firmware";
const char* update_username = "toby";
const char* update_password = "toby";
const char* ssid = "lazog";
const char* password = "abcd1234";


// DHT variabels
float humidity, temp, hi, dew;
uint32_t timerdht = 0;




ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

DHT dht(DHTPIN, DHTTYPE);






void printConstants() {
  Serial.println("garage-control");
  Serial.print("Sensor Polling interval: ");
  Serial.print(DHTREADFREQUENCY);
  Serial.print("DHT Sensor: ");
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



void setup(void){

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");

  dht.begin();
  printConstants();


  digitalWrite(RELAY1, OFF);
  digitalWrite(RELAY2, OFF);
  digitalWrite(RELAY3, OFF);
  digitalWrite(RELAY4, OFF);
  digitalWrite(RELAY5, OFF);
  digitalWrite(RELAY6, OFF);
  digitalWrite(RELAY7, OFF);
  digitalWrite(RELAY8, OFF);

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

  Serial.print("init DNS");
  MDNS.begin(host);

  Serial.println("init Webserver...");

  // handle humidity and sensor readings
  httpServer.on("/humidity", [](){
    Serial.println("HTTP read Humidity");
    char buffer[10];
    sprintf(buffer, "%f", humidity);
    httpServer.send(200, "text/plain", buffer);
  });
  httpServer.on("/temp", [](){
    Serial.println("HTTP read Temperature");
    char buffer[10];
    sprintf(buffer, "%f", temp);
    httpServer.send(200, "text/plain", buffer);
  });
  httpServer.on("/dew", [](){
    Serial.println("HTTP read Dewpoint");
    char buffer[10];
    sprintf(buffer, "%f", dew);
    httpServer.send(200, "text/plain", buffer);
  });
  httpServer.on("/hi", [](){
    Serial.println("HTTP read Heat Index");
    char buffer[10];
    sprintf(buffer, "%f", hi);
    httpServer.send(200, "text/plain", buffer);
  });


  // Handle Relay 1 .... YES this can be coded better ....
    httpServer.on("/relay1/on", [](){
      Serial.println("HTTP relay1 on");
      digitalWrite(RELAY1, ON);
      httpServer.send(200, "text/plain", "OK");
    });
    httpServer.on("/relay1/off", [](){
      Serial.println("HTTP relay1 off");
      digitalWrite(RELAY1, OFF);
      httpServer.send(200, "text/plain", "OK");
    });
    httpServer.on("/relay1/status", [](){
      Serial.println("HTTP relay1 status");
      char buffer[2];
      sprintf(buffer, "%d", digitalRead(RELAY1));
      httpServer.send(200, "text/plain", buffer);
    });









  Serial.println("start Webserver");
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
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



}
