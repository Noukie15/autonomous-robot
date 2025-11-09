#include <WiFi.h>

const char* ssid = ""; // Kweni verzin iets zoals 'PicoMotorAP'
const char* password = ""; //min 8 tekens '12345678'
WiFiServer server(80);

void setup(){
  Serial.begin(115200);
  delay(1000);

  //AP starten
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP started! IP-address: ");
  Serial.print(IP);

  //TCP-server starten
  server.begin();
}

void loop(){
  WiFiClient client =  server.available();
  if(client){
    Serial.println("Client verbonden!");

    while(client.connected()){
      if (client.available()){
        String msg = client.readStringUntil('\n');
        msg.trim();
        Serial.println("Ontvangen" + msg);

        if (msg == "MOTOR_AAN"){
          Serial.println("Motor aan");
        } else if (msg == "MOTOR_UIT"){
          Serial.println("Motor uit");
        }
      }
    }

    client.stop();
    Serial.println("Client verbroken");
  }
}
