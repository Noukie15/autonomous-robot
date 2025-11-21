/*
  Client code (control motors)
  Raspberry Pi Pico W (Arduino Core) sketch
  Ontwerpproject 2: Autonomous Robot (aka Robert de Robot)
*/

#include <WiFi.h>

const char* ssid = "PicoMotorAP";
const char* password = "12345678";
const char * serverIP = "192.168.42.1"; //default ip heb ik getest en is bij de AP pico 192.168.42.1

WiFiClient client;


// pinnen motor rechts
const int M1 = 2;
const int M2 = 3;
const int PWM1 = 4;
const int enc_pulsen1 = 9;
const int enc_rpm1 = 10;

// pinnen motor 2
const int M3 = 6;
const int M4 = 7;
const int PWM2 = 8;
const int enc_pulsen2 = 0;
const int enc_rpm2 = 1;

// ENCODER VARIABELEN
volatile int pulseCount1 = 0;
volatile int pulseCount2 = 0;

enum MotorCommand {
  CMD_STOP,
  CMD_FORWARD,
  CMD_BACKWARD,
  CMD_TURN_LEFT,
  CMD_TURN_RIGHT,
  CMD_AVOID_LEFT,
  CMD_AVOID_RIGHT,
  CMD_RECOVERY_BACK,
  CMD_END_DETECTED,
};

void executeCommand(MotorCommand command){
  switch(command) {
    case CMD_STOP:{
      motorRechtsStop();
      motorLinksStop();
      break;
                  }

    case CMD_FORWARD:{
      vooruit_rot (1, 100);
      break;
                     }

    case CMD_BACKWARD:{
      achteruit_rot(0.5, 100);
      break;
                      }

    case CMD_TURN_LEFT:{
      draailinks (1, 100);
        break;
                       }

    case CMD_TURN_RIGHT:{
      draairechts(1, 100);
      break;
                        }

    case CMD_AVOID_LEFT:{
      draailinks (1, 100);
      delay(100);
      vooruit_rot(1, 100);
                        }

    case CMD_AVOID_RIGHT:{
      draailinks (1, 100);
      delay(100);
      vooruit_rot(1, 100);
                         }

    case CMD_RECOVERY_BACK:{
      motorLinksStop();
      motorRechtsStop();
      achteruit_rot(1, 100); //niet te hard zetten voor de veiligheid
      draairechts(1, 100);
                           }

      case CMD_END_DETECTED:{
        motorLinksStop();
        motorRechtsStop();
        achteruit_rot(1, 100);
        draairechts(3, 100); //zo instellen dat de auto 180° draait
                            }
  }
}


// INTERRUPT FUNCTIES
void countPulse1() {
  pulseCount1++;
}

void countPulse2() {
  pulseCount2++;
}

void setup(){
  Serial.begin(115200);
  delay(300);

  WiFi.begin(ssid, password);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nVerbonden met Robot AP!");

  // pinmodes motorRechts
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(PWM1, OUTPUT);
  pinMode(enc_pulsen1, INPUT_PULLUP);
  pinMode(enc_rpm1, INPUT_PULLUP);

  // pinmodes motorLinks
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);
  pinMode(PWM2, OUTPUT);
  pinMode(enc_pulsen2, INPUT_PULLUP);
  pinMode(enc_rpm2, INPUT_PULLUP);

//interrupts 
  attachInterrupt(digitalPinToInterrupt(enc_pulsen1), countPulse1, RISING);
  attachInterrupt(digitalPinToInterrupt(enc_pulsen2), countPulse2, RISING);
}

void loop(){
  WiFiClient client;
  
  if (client.connect(serverIP, 80)){
    //lees inkomend commando
    if (client.available()) {
      String response = client.readStringUntil('\n');
      int commandNumber = response.toInt();

      //voer het commando uit (ik hoop dat dit werkt van de eerste keer)
      if (commandNumber >= 0 && commandNumber <= 7) { executeCommand((MotorCommand)commandNumber); } 
      else { Serial.println("ongeldig nummer ontvangen" + String(commandNumber)); }
    }
    client.stop();
  } else {
    Serial.println("Geen verbinding met AP - motoren stoppen"); // voor de veiligheid de motoren stoppen als de verbinding zou wegvallen
  }
  delay(50);
}
