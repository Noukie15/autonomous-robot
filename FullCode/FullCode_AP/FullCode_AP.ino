/*
  Access Point code (reading sensors)
  Raspberry Pi Pico W (Arduino Core) sketch
  Ontwerpproject 2: Autonomous Robot (aka Robert de Robot)
 */

//----------BIB----------
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_ADXL345_Unified.h>
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

WiFiServer server = (80);

//I2C pins (normaal heb ik die op GP0 en GP1 gezet)
const int I2C_SDA_PIN = 0;
const int I2C_SCL_PIN = 1;

//batterij spanning uitlezen setup
const int adcPin = 26;
const float R_top = 6200.0;
const float R_bottom = 4700.0;

/* ToF sensoren (niet alle sensoren werken) */
const int XSHUT_FRONT =;
const int XSHUT_LEFT =;
const int XSHUT_RIGHT =;

/* Reflectie sensoren */
const int REFL_F_LEFT =;
const int REFL_F_RIGHT =;
const int REFL_R_LEFT =;
const int REFL_R_RIGHT =;

/* Bumpers */
const int BUMPER_FL =;
const int BUMPER_FR =;
const int BUMPER_RL =;
const int BUMPER_RR =;

//----------OBJECTS----------
Adafruit_VL53L0X loxFront;
Adafruit_VL53L0X loxLeft;
Adafruit_VL53L0X loxRight;

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

//----------STATES----------
enum State {
  IDLE,
  FORWARD,
  AVOID_GAP,
  AVOID_OBSTACLE,
  END_DETECTED,
  RETURNING,
  RECOVERY,
  STOPPED
};

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
}

State state = IDLE;

//----------GLOBALS----------
#define REF_GROUND HIGH
#define REF_GAP LOW

const uint16_t OBSTACLE_THRESH = 300;
const uint16_t TOF_OPEN_THRESH = 2000;

//----------SENSOR HELPERS----------

bool anyBumper() {
  return (digitalRead(BUMPER_FL)==LOW ||
          digitalRead(BUMPER_FR)==LOW ||
          digitalRead(BUMPER_RL)==LOW ||
          digitalRead(BUMPER_RR)==LOW);
}

uint16_t readTof() {
  VL53L0X_RangingMeasurementData_t m;
  lox.rangingTest(&m, false);
  if (m.RangeStatus == 4) return 0xFFFF;
  return m.RangeMilliMeter;
}

bool frontGap() {
  return (digitalRead(REFL_F_LEFT)==REF_GAP &&
          digitalRead(REFL_F_RIGHT)==REF_GAP);
}

bool frontGround() {
  return (digitalRead(REFL_F_LEFT)==REF_GROUND ||
          digitalRead(REFL_F_RIGHT)==REF_GROUND);
}

bool rearGround() {
  return (digitalRead(REFL_R_LEFT)==REF_GROUND ||
          digitalRead(REFL_R_RIGHT)==REF_GROUND);
}

float headingError() {
  sensors_event_t e;
  accel.getEvent(&e);
  return headingRef - e.acceleration.x;
}

//----------MOTOR COMMUNICATION FUNCTION----------
void sendCommand(MotorCommand cmd){
  WiFiClient client = server.accept();

  if (client) {
    //het commando versturen als een zijn nummer
    client.println((int)cmd);

    unsigned long timeout = millis();
    while (client.connected() && client.accept() ==0 ){
      if (millis() - timeout > 500) break;
    }
    client.stop();
  } else {
    Serial.println("blijkbaar is de client niet verbonden voor motor commands");
  }
}
//----------EXTRA FUNCTIONS----------
void readBattery(){
  int adcValue = analogRead(adcPin);
  float adcVoltage = (adcValue / 1023.0) * 3.3;
  float batteryVoltage = adcVolatge * (R_top + R_bottom) / R_bottom;

  // OLED update
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Batterijspanning:");

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(batteryVoltage, 2);
  display.println(" V");

    display.display();

}
//----------SETUP----------
void setup() {
  Serial.begin(115200);
  delay(300);

  //AP starten
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("AP gestart op: ");
  Serial.println(IP);

  server.begin();
  
  //zet I2C op GP0/GP1
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED niet gevonden op 0x3C!");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.display();

  // Sensors
  pinMode(REFL_F_LEFT, INPUT);
  pinMode(REFL_F_RIGHT, INPUT);
  pinMode(REFL_R_LEFT, INPUT);
  pinMode(REFL_R_RIGHT, INPUT);

  pinMode(BUMPER_FL, INPUT_PULLUP);
  pinMode(BUMPER_FR, INPUT_PULLUP);
  pinMode(BUMPER_RL, INPUT_PULLUP);
  pinMode(BUMPER_RR, INPUT_PULLUP);

  //accelerometer
  accel.begin();
  accel.setRange(ADXL345_RANGE_16_G);

  float accSum = 0;
  for (int i = 0 ; i < 10 ; i++){
    sensors_event_t ev;
    accel.getEvent(&ev);
    accSum += ev.acceleration.x;
    delay(20);
  }
  headingRef = accSum / 10.0;

  //Tof XSHUT
  pinMode(XHUT_FRONT, OUTPUT);
  pinMode(XHUT_LEFT, OUTPUT);
  pinMode(XHUT_RIGHT, OUTPUT);

  digitalWrite(XSHUT_FRONT, LOW);
  digitalWrite(XSHUT_LEFT, LOW);
  digitalWrite(XSHUT_RIGHT, LOW);
  delay(10);

  digitalWrite(XSHUT_FRONT, HIGH);
  loxFront.begin();
  delay(10);

  digitalWrite(XSHUT_LEFT, HIGH);
  loxLeft.begin();
  delay(10);

  digitalWrite(XSHUT_RIGHT, HIGH);
  loxRight.begin();
  
  state = FORWARD;
}

//----------MAIN LOOP----------
void loop(){
  
  WiFiClient client = server.accept();

  uint16_t dF = readToF(loxFront);
  uint16_t dL = readToF(loxLeft);
  uint16_t dR = readToF(loxRight);

  bool bumper = anyBumper();

  if (bumper && state != RECOVERY){
    state = RECOVERY;
    Serial.println("RECOVERY want er werd een bumper ingedrukt!");
  }

  switch(state){

    case FORWARD: {
      if (frontGap()){
        state  = AVOID_GAP;
        break;
      }
      if (dF < OBSTACLE_THRESH && dF != 0xFFFF){
        state = AVOID_OBSTACLE;
        break;
      }
      //end detection
      bool tofOpen = (dF >= TOF_OPENTHRESH || dF == 0xFFFF);
      bool flatFront = frontGap();
      sensors_event_t e;
      accel.getEvent(&e);
      bool slope = (abs(e.acceleration.x - headingRef) > 0.5);

      int conf = tofOpen + flatFront + slope;
      if (conf == 3) {
        state = END_DETECTED;
        break;
      }
      sendCommand(CMD_FORWARD);
      break;
                  }

    case AVOID_GAP: {
      bool leftGround = (digitalRead(REFL_F_LEFT) == REF_GROUND);
      bool rightGround = (digitalRead(REFL_F_RIGHT) == REF_GROUND);

      if (leftGround && !rightGround) {
        sendCommand(CMD_AVOID_RIGHT);
        state = FORWARD;
      } else if (rightGround && !leftGround) {
        sendCommand(CMD_AVOID_LEFT);
        state = FORWARD;
      } else if (leftGround && rightGround) {
        if (dL > dR) sendCommand(CMD_AVOID_RIGHT);
        else sendCommand(CMD_AVOID_RIGHT);
        state = FORWARD;
      } else {
        // worst case -> kant kiezen die het veiligst is: degene met de grootste ToF
        if (dL > dR) sendCommand(CMD_AVOID_LEFT);
        else sendCommand(CMD_AVOID_RIGHT);
        state = FORWARD;
      }
      break;
                    }

    case AVOID_OBSTACLE: {
      // beste diagonaal kiezen
      if (dL > dR +50){
        sendCommand(CMD_AVOID_LEFT);
      } else if (dR > dL + 50){
        sendCommand(CMD_AVOID_RIGHT);
      } else {
        //geen duidelijke diagonaal -> kies de grootste opening
        if (dL > dR) sendCommand(CMD_AVOID_LEFT);
        else sendCommand(CMD_AVOID_RIGHT);
      }
      state = FORWARD;
      break;
                         }

    case END_DETECTED: {
      sendCommand(CMD_END_DETECTED);
      state = RETURNING;
      break;
                        }

    case RETURNING: {
      if (frontGap()) { state ==  AVOID_GAP; break; }
      if (dF < OBSTACLE_TRESH && dF != 0xFFFF) { state = AVOID_OBSTACLE; break; }

      if (rearGround() && (dF >= TOF_OPEN_THRESH || dF == 0xFFFF)){
        sendCommand(CMD_STOP);
        state = STOPPED;
        break;
      }
      sendCommand(CMD_FORWARD);
      break;
                    }

    case RECOVERY: {
      sendCommand(CMD_RECOVERY_BACK);
      state = FORWARD;
      break;
                   }

    case STOPPED: {
      sendCommand(CMD_STOP);
      break;
                  }

  }

  delay(20);
}
