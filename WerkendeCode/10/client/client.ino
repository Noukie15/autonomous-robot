/*
  Client code (control motors) - Raspberry Pi Pico W 2
*/

#include <WiFi.h>

const char* ssid = "PicoMotorAP";
const char* password = "12345678";
const char * serverIP = "192.168.42.1";

// wifi client
WiFiClient client;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_INTERVAL = 10000;
bool wasConnected = false;
int connectionAttempts = 0;
const int MAX_ATTEMPTS = 10;

// Verbindingsstatus variabelen
bool isConnectedToServer = false;

// ---------------- INSTELLINGEN ----------------
#define MOTOR_SPEED 140
#define MOTOR_SPEED_FAST 100  // Hogere snelheid voor gat vermijding
#define MOTOR_SPEED_SOFT 145   // Zachte snelheid voor bochten
#define MANEUVER_DELAY 350
#define BACKWARD_DELAY 250
#define FORWARD_DELAY 150
#define DOUBLE_BACKWARD_DELAY 700  // Langer achteruit voor gaten
#define BUMPER_BACKWARD_DELAY 200
#define TURN_180_DELAY 1200
#define GAP_BACKWARD_DELAY 400     // Langer achteruit voor gat detectie
#define GAP_TURN_DELAY 450         // Langere bocht voor gat detectie
#define PARTIAL_GAP_BACKWARD_DELAY 150  // Korte achteruit voor partiële gaten

// pinnen motor rechts
const int M1_DIR1 = 6;
const int M1_DIR2 = 7;
const int M1_PWM = 8;

// pinnen motor links
const int M2_DIR1 = 2;
const int M2_DIR2 = 3;
const int M2_PWM = 4;

// Encoder pinnen (indien gebruikt)
const int enc_pulsen1 = 9;
const int enc_pulsen2 = 10;

// Encoder variabelen
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

bool connectToWiFi() {
  Serial.println("Initialiseren WiFi...");
  WiFi.mode(WIFI_STA);
  delay(100);
  
  Serial.print("Verbinden met '");
  Serial.print(ssid);
  Serial.println("'...");
  
  WiFi.begin(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {
      Serial.println("Timeout: WiFi verbinding mislukt");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi verbonden!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool connectToServer() {
  Serial.print("Verbinden met server ");
  Serial.print(serverIP);
  Serial.println(":80...");
  
  if (client.connect(serverIP, 80)) {
    Serial.println("Server verbonden!");
    wasConnected = true;
    connectionAttempts = 0;
    isConnectedToServer = true;
    return true;
  } else {
    Serial.println("Server verbinding mislukt!");
    stopMotors();
    isConnectedToServer = false;
    return false;
  }
}

// Encoder interrupt handlers
void countPulse1() {
  pulseCount1++;
}

void countPulse2() {
  pulseCount2++;
}

//basis motorfuncties
void moveForward() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: moveForward");
  // Motor links vooruit
  digitalWrite(M1_DIR1, LOW);
  digitalWrite(M1_DIR2, HIGH);
  analogWrite(M1_PWM, MOTOR_SPEED);
 
  // Motor rechts vooruit
  digitalWrite(M2_DIR1, HIGH);
  digitalWrite(M2_DIR2, LOW);
  analogWrite(M2_PWM, MOTOR_SPEED-35);
}

void moveBackward() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: moveBackward");
  // Motor links achteruit
  digitalWrite(M1_DIR1, HIGH);
  digitalWrite(M1_DIR2, LOW);
  analogWrite(M1_PWM, MOTOR_SPEED);
 
  // Motor rechts achteruit
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, HIGH);
  analogWrite(M2_PWM, MOTOR_SPEED-35);
}

void moveBackwardFast() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: moveBackwardFast");
  // Motor links achteruit - SNEL
  digitalWrite(M1_DIR1, HIGH);
  digitalWrite(M1_DIR2, LOW);
  analogWrite(M1_PWM, MOTOR_SPEED_FAST);
 
  // Motor rechts achteruit - SNEL
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, HIGH);
  analogWrite(M2_PWM, MOTOR_SPEED_FAST-35);
}

// ZACHTERE BOCHT FUNCTIES
void turnRightSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnRightSoft");
  // Motor links vooruit - ZACHTER
  digitalWrite(M1_DIR1, LOW);
  digitalWrite(M1_DIR2, HIGH);
  analogWrite(M1_PWM, MOTOR_SPEED_SOFT);
 
  // Motor rechts achteruit - ZACHTER
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, HIGH);
  analogWrite(M2_PWM, MOTOR_SPEED_SOFT-20);
}

void turnLeftSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnLeftSoft");
  // Motor links achteruit - ZACHTER
  digitalWrite(M1_DIR1, HIGH);
  digitalWrite(M1_DIR2, LOW);
  analogWrite(M1_PWM, MOTOR_SPEED_SOFT);
 
  // Motor rechts vooruit - ZACHTER
  digitalWrite(M2_DIR1, HIGH);
  digitalWrite(M2_DIR2, LOW);
  analogWrite(M2_PWM, MOTOR_SPEED_SOFT-20);
}

// Bestaande bocht functies (voor snelle manoeuvres)
void turnRight() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnRight");
  // Motor links vooruit
  digitalWrite(M1_DIR1, LOW);
  digitalWrite(M1_DIR2, HIGH);
  analogWrite(M1_PWM, MOTOR_SPEED);
 
  // Motor rechts achteruit
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, HIGH);
  analogWrite(M2_PWM, MOTOR_SPEED-35);
}

void turnRightFast() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnRightFast");
  // Motor links vooruit - SNEL
  digitalWrite(M1_DIR1, LOW);
  digitalWrite(M1_DIR2, HIGH);
  analogWrite(M1_PWM, MOTOR_SPEED_FAST);
 
  // Motor rechts achteruit - SNEL
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, HIGH);
  analogWrite(M2_PWM, MOTOR_SPEED_FAST-35);
}

void turnLeft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnLeft");
  // Motor links achteruit
  digitalWrite(M1_DIR1, HIGH);
  digitalWrite(M1_DIR2, LOW);
  analogWrite(M1_PWM, MOTOR_SPEED);
 
  // Motor rechts vooruit
  digitalWrite(M2_DIR1, HIGH);
  digitalWrite(M2_DIR2, LOW);
  analogWrite(M2_PWM, MOTOR_SPEED-35);
}

void turnLeftFast() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: turnLeftFast");
  // Motor links achteruit - SNEL
  digitalWrite(M1_DIR1, HIGH);
  digitalWrite(M1_DIR2, LOW);
  analogWrite(M1_PWM, MOTOR_SPEED_FAST);
 
  // Motor rechts vooruit - SNEL
  digitalWrite(M2_DIR1, HIGH);
  digitalWrite(M2_DIR2, LOW);
  analogWrite(M2_PWM, MOTOR_SPEED_FAST-35);
}

void stopMotors() {
  Serial.println("Uitvoeren: stopMotors");
  analogWrite(M1_PWM, 255);
  analogWrite(M2_PWM, 255);
}

// NIEUWE FUNCTIE VOOR RECHTE GAT DETECTIE
void executeManeuverDoubleBackRight() {
  if (!isConnectedToServer) return;
  Serial.println("UITVOEREN: executeManeuverDoubleBackRight - RECHT OP GAT!");
  
  // Stap 1: EXTRA LANG en SNEL achteruit voor recht gat
  moveBackwardFast();
  delay(DOUBLE_BACKWARD_DELAY);
  stopMotors();
  delay(50);
 
  // Stap 2: ZACHTER rechts draaien
  turnRightSoft();
  delay(GAP_TURN_DELAY + 100); // Iets langere maar zachtere bocht
  stopMotors();
  delay(50);
}

// NIEUWE FUNCTIES VOOR PARTIËLE GAT DETECTIE MET ACHTERUIT
void executeManeuverBackRightSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverBackRightSoft - ACHTERUIT + ZACHTER");
  
  // Stap 1: Korte achteruit voor veiligheid
  moveBackward();
  delay(PARTIAL_GAP_BACKWARD_DELAY);
  stopMotors();
  delay(50);
  
  // Stap 2: Zachte bocht naar rechts
  turnRightSoft();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(50);
 /*
  // Stap 3: Vooruit rijden
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(50);*/
}

void executeManeuverBackLeftSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverBackLeftSoft - ACHTERUIT + ZACHTER");
  
  // Stap 1: Korte achteruit voor veiligheid
  moveBackward();
  delay(PARTIAL_GAP_BACKWARD_DELAY);
  stopMotors();
  delay(50);
  
  // Stap 2: Zachte bocht naar links
  turnLeftSoft();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(50);
 /*
  // Stap 3: Vooruit rijden
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(50);*/
}

// NIEUWE ZACHTERE MANOEUVRE FUNCTIES (ZONDER ACHTERUIT)
void executeManeuverRightForwardNoBackSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverRightForwardNoBackSoft - ZACHTER");
  
  turnRightSoft();
  delay(MANEUVER_DELAY); // Langere maar zachtere bocht
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

void executeManeuverLeftForwardNoBackSoft() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverLeftForwardNoBackSoft - ZACHTER");
  
  turnLeftSoft();
  delay(MANEUVER_DELAY); // Langere maar zachtere bocht
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

// Bestaande manoeuvre functies
void executeManeuver180Turn() {
  if (!isConnectedToServer) return;
  Serial.println("UITVOEREN: executeManeuver180Turn");
 
  stopMotors();
  delay(100);
  moveBackward();
  delay(150);
  stopMotors();
  delay(100);
 
  turnRight();
  delay(TURN_180_DELAY);
  stopMotors();
  delay(100);
 
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);
}

void executeManeuverBackRightForward() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverBackRightForward");
  
  moveBackward();
  delay(BACKWARD_DELAY);
  stopMotors();
  delay(100);
 
  turnRight();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

void executeManeuverBackLeftForward() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverBackLeftForward");
  
  moveBackward();
  delay(BACKWARD_DELAY);
  stopMotors();
  delay(100);
 
  turnLeft();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

void executeManeuverRightForwardNoBack() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverRightForwardNoBack");
  
  turnRight();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

void executeManeuverLeftForwardNoBack() {
  if (!isConnectedToServer) return;
  Serial.println("Uitvoeren: executeManeuverLeftForwardNoBack");
  
  turnLeft();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);
 /*
  moveForward();
  delay(MANEUVER_DELAY);
  stopMotors();
  delay(100);*/
}

void executeCommand(const String& command) {
  if (!isConnectedToServer) return;
  
  Serial.print("Ontvangen commando: ");
  Serial.println(command);

  if (command == "stopMotors") {
    stopMotors();
  } else if (command == "moveForward") {
    moveForward();
  } else if (command == "moveBackward") {
    moveBackward();
  } else if (command == "turnRight") {
    turnRight();
  } else if (command == "turnLeft") {
    turnLeft();
  } else if (command == "executeManeuverBackRightForward") {
    executeManeuverBackRightForward();
  } else if (command == "executeManeuverBackLeftForward") {
    executeManeuverBackLeftForward();
  } else if (command == "executeManeuverRightForwardNoBack") {
    executeManeuverRightForwardNoBack();
  } else if (command == "executeManeuverLeftForwardNoBack") {
    executeManeuverLeftForwardNoBack();
  } else if (command == "executeManeuver180Turn") {
    executeManeuver180Turn();
  } 
  // NIEUW COMMANDO VOOR RECHTe GAT DETECTIE
  else if (command == "executeManeuverDoubleBackRight") {
    executeManeuverDoubleBackRight();
  }
  // NIEUWE ZACHTERE COMMANDO'S
  else if (command == "executeManeuverRightForwardNoBackSoft") {
    executeManeuverRightForwardNoBackSoft();
  } else if (command == "executeManeuverLeftForwardNoBackSoft") {
    executeManeuverLeftForwardNoBackSoft();
  }
  // NIEUWE COMMANDO'S MET ACHTERUIT VOOR PARTIËLE GATEN
  else if (command == "executeManeuverBackRightSoft") {
    executeManeuverBackRightSoft();
  } else if (command == "executeManeuverBackLeftSoft") {
    executeManeuverBackLeftSoft();
  } else {
    Serial.print("Onbekend commando: ");
    Serial.println(command);
  }
}

void handleServerCommands() {
  if (client.connected() && client.available()) {
    String command = client.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      executeCommand(command);
    }
  }
}

void checkConnection() {
  if (!client.connected()) {
    isConnectedToServer = false;
    if (millis() - lastConnectionAttempt > CONNECTION_INTERVAL) {
      Serial.println("Verbinding verbroken, opnieuw verbinden...");
      if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
      }
      connectToServer();
      lastConnectionAttempt = millis();
      connectionAttempts++;
      
      if (connectionAttempts > MAX_ATTEMPTS) {
        Serial.println("Te veel verbindingspogingen, stop motoren en wacht...");
        stopMotors();
        connectionAttempts = 0;
      }
    }
  } else {
    connectionAttempts = 0;
    isConnectedToServer = true;
  }
}

void setup(){
  Serial.begin(115200);
  
  // CRITIEKE DELAY voor externe voeding
  delay(5000);
  Serial.println("=== ROBOT CLIENT START ===");
  
  // pinmodes motorRechts
  pinMode(M1_DIR1, OUTPUT);
  pinMode(M1_DIR2, OUTPUT);
  pinMode(M1_PWM, OUTPUT);

  // pinmodes motorLinks
  pinMode(M2_DIR1, OUTPUT);
  pinMode(M2_DIR2, OUTPUT);
  pinMode(M2_PWM, OUTPUT);

  // Encoder pinnen
  pinMode(enc_pulsen1, INPUT_PULLUP);
  pinMode(enc_pulsen2, INPUT_PULLUP);

  //interrupts
  attachInterrupt(digitalPinToInterrupt(enc_pulsen1), countPulse1, RISING);
  attachInterrupt(digitalPinToInterrupt(enc_pulsen2), countPulse2, RISING);

  Serial.println("Initialisatie voltooid - Start verbinding...");
  if (connectToWiFi()) {
    connectToServer();
  }
  
  Serial.println("Setup voltooid");
}

void loop(){
  checkConnection();
  
  if (isConnectedToServer) {
    handleServerCommands();
  } else {
    stopMotors();
  }
  
  delay(10);
}