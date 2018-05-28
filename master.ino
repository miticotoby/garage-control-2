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


#define RELAY1 8
#define RELAY2 7
#define RELAY3 6
#define RELAY4 5
#define RELAY5 4
#define RELAY6 3
#define RELAY7 2
#define RELAY8 1


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "DHT.h"

#define OFF LOW
#define ON HIGH

#define DHTTYPE DHT22
#define DHTPIN 0
#define DHTREADFREQUENCY 10000    // read once every 10 sec
#define DHTFAILFREQUENCY  3000    // if read failes try again after 3 seconds


// Wifi/Webserver variables
const char* host = "esp8266-webupdate";
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
  printConstants();
  dht.begin();


  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY7, OUTPUT);
  pinMode(RELAY8, OUTPUT);


  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  MDNS.begin(host);

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
