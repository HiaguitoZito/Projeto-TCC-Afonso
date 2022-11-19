#ifdef ESP32
  #include "WiFi.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include "ESPAsyncWebServer.h"
#include "SPI.h"

const char* ssid = "ESPsoftAP_01";
const char* password = "pass-to-soft-AP";
WiFiClient client;

IPAddress server(192,168,101,101);

IPAddress ip(192,168,101,2);
IPAddress gateway(192,168,101,1);
IPAddress subnet(255,255,255,0);

void setup() {
  Serial.begin(115200);
  Serial.println("Serial connected");
  pinMode(A0,INPUT);
  WiFi.softAPConfig(ip,gateway,subnet);
  boolean result = WiFi.softAP("ESPsoftAP_01", "12345678");
  if(result) Serial.println("Created");
  else Serial.println("Createdn't");
  
}
int* value = new int[1000];
int i = 0;
void loop() {
  if(i == 999){
    int average = 0;
    for(int b = 0; b < 1000; b++){
      average += value[b];
    }
    average /= 1000;
    client.connect(server,80);
    client.print(String(average)+"\r");
    client.flush();
    i = 0;
  } else{
    value[i] = analogRead(A0);
    i++;
  }
  
}
