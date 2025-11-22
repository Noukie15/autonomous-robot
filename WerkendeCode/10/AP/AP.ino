/*
  Access Point code (reading sensors) - Raspberry Pi Pico W 2
  Ontwerpproject 2: Autonomous Robot (aka Robert de Robot)
 */

//----------BIB----------
#include <Wire.h>
#include <VL53L0X.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//connection setup
const char* ssid = "PicoMotorAP";
const char* password = "12345678";

WiFiServer server(80);
WiFiClient globalClient;
bool clientConnected = false;
bool readyToStart = false;
unsigned long connectionEstablishedTime = 0;
const unsigned long START_DELAY = 2000;

//I2C pins
const int I2C_SDA_PIN = 0;
const int I2C_SCL_PIN = 1;

/* ToF sensoren */
const int XSHUT_RIGHT = 3;  // Schuin rechts vooraan
const int XSHUT_LEFT = 4;   // Schuin links vooraan

/* Reflectie sensoren */
const int REFL_F_LEFT = 11;
const int REFL_F_RIGHT = 12;
const int REFL_R_LEFT = 9;
const int REFL_R_RIGHT = 10;

/* Bumpers */
const int BUMPER_FL = 6;
const int BUMPER_FR = 5;
const int BUMPER_RL = 8;
const int BUMPER_RR = 7;

//----------OBJECTS----------
VL53L0X sensRight;  // Schuin rechts vooraan
VL53L0X sensLeft;   // Schuin links vooraan

//----------GLOBALS----------
#define REF_GROUND LOW
#define REF_GAP HIGH

const uint16_t OBSTACLE_THRESH = 100;  // Afstand in mm voor obstakeldetectie
const uint16_t TOF_OPEN_THRESH = 200;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 50;

// Detectie variabelen
bool gapDetected = false;
bool obstacleDetected = false;
bool bumperDetected = false;
bool leftObstacle = false;
bool rightObstacle = false;

//----------OLED DISPLAY FUNCTIONS----------
void displayClientConnected() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Status: AP Actief");
  display.setCursor(0, 20);
  display.println("Client verbonden!");
  display.setCursor(0, 40);
  display.println("Robert is ready!");
  display.display();
}

void displayWaitingForClient() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Status: AP Actief");
  display.setCursor(0, 20);
  display.println("Wacht op client...");
  display.setCursor(0, 40);
  display.println("SSID: PicoMotorAP");
  display.display();
}

void displayConnectionEstablished() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Status: AP Actief");
  display.setCursor(0, 20);
  display.println("Client verbonden!");
  display.setCursor(0, 40);
  display.println("Start over 2 sec...");
  display.display();
}

void displaySensorInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  
  // ToF afstanden
  display.setCursor(0, 0);
  display.print("ToF R: ");
  display.print(readToF(sensRight));
  display.print("mm");
  display.setCursor(0, 10);
  display.print("ToF L: ");
  display.print(readToF(sensLeft));
  display.print("mm");
  
  // Obstakel status
  display.setCursor(0, 20);
  display.print("Obst R: ");
  display.print(rightObstacle ? "YES" : "NO");
  display.setCursor(70, 20);
  display.print("L: ");
  display.print(leftObstacle ? "YES" : "NO");
  
  // IR sensor status
  bool leftGap = digitalRead(REFL_F_LEFT) == REF_GAP;
  bool rightGap = digitalRead(REFL_F_RIGHT) == REF_GAP;
  
  display.setCursor(0, 30);
  display.print("Gap L:");
  display.print(leftGap ? "YES" : "NO");
  display.setCursor(70, 30);
  display.print("R:");
  display.print(rightGap ? "YES" : "NO");
  
  // Algemene detectie status
  display.setCursor(0, 40);
  display.print("Gap Detected: ");
  display.print(gapDetected ? "YES" : "NO");
  display.setCursor(0, 50);
  display.print("Obstacle: ");
  display.print(obstacleDetected ? "YES" : "NO");
  display.setCursor(0, 60);
  display.print("Bumper: ");
  display.print(bumperDetected ? "YES" : "NO");
  
  display.display();
}

//----------SENSOR HELPERS----------
bool anyBumper() {
  bumperDetected = (digitalRead(BUMPER_FL) == LOW ||
                   digitalRead(BUMPER_FR) == LOW ||
                   digitalRead(BUMPER_RL) == LOW ||
                   digitalRead(BUMPER_RR) == LOW);
  return bumperDetected;
}

uint16_t readToF(VL53L0X &sensor) {
  uint16_t d = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred()) {
    return 0xFFFF;
  }
  return d;
}

bool checkGap() {
  bool leftGap = digitalRead(REFL_F_LEFT) == REF_GAP;
  bool rightGap = digitalRead(REFL_F_RIGHT) == REF_GAP;
  
  // Gat gedetecteerd als minstens één sensor een gat ziet
  gapDetected = leftGap || rightGap;
  
  return gapDetected;
}

bool checkObstacle() {
  uint16_t rightDist = readToF(sensRight);
  uint16_t leftDist = readToF(sensLeft);
  
  // Individuele obstakel detectie
  rightObstacle = (rightDist < OBSTACLE_THRESH && rightDist != 0xFFFF);
  leftObstacle = (leftDist < OBSTACLE_THRESH && leftDist != 0xFFFF);
  
  // Algemene obstakel detectie - als minstens één sensor een obstakel ziet
  obstacleDetected = rightObstacle || leftObstacle;
  
  return obstacleDetected;
}

//----------MOTOR COMMUNICATION FUNCTION----------
void sendCommand(const String& command) {
  if (globalClient && globalClient.connected() && readyToStart) {
    globalClient.println(command);
    Serial.print("Command sent: ");
    Serial.println(command);
    delay(5);
  }
}

void checkClientConnection() {
  bool currentState = (globalClient && globalClient.connected());
  
  if (currentState != clientConnected) {
    clientConnected = currentState;
    if (clientConnected) {
      displayConnectionEstablished();
      connectionEstablishedTime = millis();
      Serial.println("Client verbonden! Start over 2 seconden...");
    } else {
      displayWaitingForClient();
      readyToStart = false;
      Serial.println("Client verbroken!");
    }
  }
  
  if (clientConnected && !readyToStart && (millis() - connectionEstablishedTime > START_DELAY)) {
    readyToStart = true;
    displayClientConnected();
    Serial.println("Klaar om te starten!");
  }
}

void readSensors() {
  if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
    checkGap();
    checkObstacle();
    anyBumper();
    
    // Debug output
    Serial.print("Gap: ");
    Serial.print(gapDetected);
    Serial.print(" | Obstacle: ");
    Serial.print(obstacleDetected);
    Serial.print(" | L_Obst: ");
    Serial.print(leftObstacle);
    Serial.print(" | R_Obst: ");
    Serial.print(rightObstacle);
    Serial.print(" | Bumper: ");
    Serial.print(bumperDetected);
    Serial.print(" | ToF R: ");
    Serial.print(readToF(sensRight));
    Serial.print(" | ToF L: ");
    Serial.println(readToF(sensLeft));
    
    displaySensorInfo();
    lastSensorRead = millis();
  }
}

void handleRobotLogic() {
  if (!readyToStart) return;
  
  // Prioriteit: Eerst gaten, dan bumpers, dan obstakels
  if (gapDetected) {
    bool leftGap = digitalRead(REFL_F_LEFT) == REF_GAP;
    bool rightGap = digitalRead(REFL_F_RIGHT) == REF_GAP;

    // Alleen naar achteren rijden als beide sensoren een gat detecteren (recht vooruit)
    if (leftGap && rightGap) {
      // RECHT OP GAT AF - SNELLE ACHTERUIT MANOEUVRE
      Serial.println("RECHT OP GAT! - SNELLE ACHTERUIT");
      sendCommand("executeManeuverDoubleBackRight");
    } 
    else if (leftGap && !rightGap) {
      // Alleen links gat - EERST ACHTERUIT, dan zachter naar rechts
      Serial.println("Links gat - ACHTERUIT + zachter rechts");
      sendCommand("executeManeuverBackRightSoft");
    } 
    else if (!leftGap && rightGap) {
      // Alleen rechts gat - EERST ACHTERUIT, dan zachter naar links
      Serial.println("Rechts gat - ACHTERUIT + zachter links");
      sendCommand("executeManeuverBackLeftSoft");
    }
    
    // Wacht even om te voorkomen dat we te snel opeenvolgende commando's sturen
    delay(800);
  }
  else if (bumperDetected) {
    Serial.println("BUMPER DETECTED - Recovery manoeuvre");
    sendCommand("moveBackward");
    delay(500);
    sendCommand("stopMotors");
    delay(200);
    // Gebruik manoeuvre met achteruit voor bumper recovery
    sendCommand("executeManeuverBackRightSoft");
    delay(800);
  }
  else if (obstacleDetected) {
    Serial.println("OBSTACLE DETECTED - Veilige obstacle avoidance");
    
    // Gebruik de individuele ToF metingen om de beste richting te kiezen
    if (rightObstacle && !leftObstacle) {
      // Alleen rechts obstakel - ACHTERUIT + zachter naar links
      Serial.println("Right obstacle only - ACHTERUIT + zachter links");
      sendCommand("executeManeuverBackLeftSoft");
    }
    else if (!rightObstacle && leftObstacle) {
      // Alleen links obstakel - ACHTERUIT + zachter naar rechts
      Serial.println("Left obstacle only - ACHTERUIT + zachter rechts");
      sendCommand("executeManeuverBackRightSoft");
    }
    else if (rightObstacle && leftObstacle) {
      // Beide kanten hebben obstakels - kies de kant met het verst verwijderde obstakel
      uint16_t leftDist = readToF(sensLeft);
      uint16_t rightDist = readToF(sensRight);
      
      if (leftDist > rightDist) {
        Serial.println("Both obstacles - left side clearer, ACHTERUIT + zachter links");
        sendCommand("executeManeuverBackLeftSoft");
      } else {
        Serial.println("Both obstacles - right side clearer, ACHTERUIT + zachter rechts");
        sendCommand("executeManeuverBackRightSoft");
      }
    }
    else {
      // Fallback - ACHTERUIT + zachter naar rechts
      Serial.println("Fallback obstacle avoidance - ACHTERUIT + zachter rechts");
      sendCommand("executeManeuverBackRightSoft");
    }
    
    delay(800);
  }
  else {
    // Geen obstakels - gewoon vooruit rijden
    sendCommand("moveForward");
  }
}

//----------SETUP----------
void setup() {
  Serial.begin(115200);
  delay(300);
  
  // I2C op GP0/GP1
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Wire.begin();

  // OLED initialisatie
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED niet gevonden op 0x3C!");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Startup...");
  display.setCursor(0, 20);
  display.println("Initialiseer AP");
  display.display();
  delay(1000);

  // AP starten
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Gestart!");
  display.setCursor(0, 15);
  display.print("IP: ");
  display.println(IP);
  display.setCursor(0, 30);
  display.println("Wacht op client...");
  display.display();

  Serial.println("AP gestart op: ");
  Serial.println(IP);

  server.begin();

  // Sensors
  pinMode(REFL_F_LEFT, INPUT);
  pinMode(REFL_F_RIGHT, INPUT);
  pinMode(REFL_R_LEFT, INPUT);
  pinMode(REFL_R_RIGHT, INPUT);

  pinMode(BUMPER_FL, INPUT_PULLUP);
  pinMode(BUMPER_FR, INPUT_PULLUP);
  pinMode(BUMPER_RL, INPUT_PULLUP);
  pinMode(BUMPER_RR, INPUT_PULLUP);

  // ToF XSHUT - nu beide schuin vooraan
  pinMode(XSHUT_RIGHT, OUTPUT);  // Schuin rechts
  pinMode(XSHUT_LEFT, OUTPUT);   // Schuin links

  // Initialiseer ToF sensoren
  digitalWrite(XSHUT_RIGHT, LOW);
  digitalWrite(XSHUT_LEFT, LOW);
  delay(10);

  // Start rechts sensor eerst
  digitalWrite(XSHUT_RIGHT, HIGH);
  delay(10);
  sensRight.init(true);
  sensRight.setAddress(0x30);
  sensRight.startContinuous(30);

  // Start links sensor
  digitalWrite(XSHUT_LEFT, HIGH);
  delay(10);
  sensLeft.init(true);
  sensLeft.setAddress(0x31);
  sensLeft.startContinuous(30);

  Serial.println("ToF sensoren geinitialiseerd:");
  Serial.println("- Rechts: schuin rechts vooraan (0x30)");
  Serial.println("- Links: schuin links vooraan (0x31)");

  displayWaitingForClient();
}

//----------MAIN LOOP----------
void loop() {
  // Check client verbinding
  if (!globalClient || !globalClient.connected()) {
    globalClient = server.accept();
  }
  
  checkClientConnection();
  readSensors();
  handleRobotLogic();

  delay(10);
}