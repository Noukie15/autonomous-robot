#include <WiFi.h>

const char* ssid = ""; //zelfde als AP
const char* password = ""; //idem
const char* serverIP = ""; //default IP van Pico is 192.168.4.1

WiFiClient client;

void setup(){
  Serial.begin(115200);
  delay(1000);

  Serial.println("Verbinding maken met AP...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nVerbonden met AP!");
  Serial.print("IP-adres van client: ");
  Serial.println(WiFi.localIP());
}

void loop(){
  if (client.connect(serverIP, 80)){
    Serial.println("Verbonden met server, commando sturen...");
    client.println("MOTOR_AAN");
    client.stop();
  } else {
    Serial.println("Kon niet verbinden met server!!") //Oh shit not good :'(
  }

  delay(2000);
  if (client.connect(serverIP, 80)){
    Serial.println("Verbinding voor MOTOR_UIT...");
    client.println("MOTOR_UIT");
    client.stop();
  }

  delay(2000); //Pls help I can't stop
}
