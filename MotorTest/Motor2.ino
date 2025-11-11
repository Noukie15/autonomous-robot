#define M1_DIR1 2
#define M1_DIR2 3
#define M1_PWM 4

#define M2_DIR1 6
#define M2_DIR2 7
#define M2_PWM 8

const int pwmResolution = 8;
int duty = 0;

void setup(){
  Serial.begin(115200);
  delay(3000);
  Serial.println("Start H-brug voor 2 motoren!");
  pinMode(M1_DIR1, OUTPUT);
  pinMode(M1_DIR2, OUTPUT);
  pinMode(M1_PWM, OUTPUT);
  pinMode(M2_DIR1, OUTPUT);
  pinMode(M2_DIR2, OUTPUT);
  pinMode(M2_PWM, OUTPUT);

  stopMotor1();
  stopMotor2();
}

void loop(){
  // --- Motor 1---
  Serial.println("Motor 1 vooruit!");
  setDirectionMotor1(true);
  rampPWM(M1_PWM, 0, 255, 5);
  delay(2000);
  rampPWM(M1_PWM, 255, 0, 5);

  delay(1000);

  Serial.println("Motor 1 achteruit!");
  setDirectionMotor1(false);
  rampPWM(M1_PWM, 0, 255, 5);
  delay(2000);
  rampPWM(M1_PWM, 255, 0, 5);
  stopMotor1();

  delay(1000);

  // --- Motor 2---
  Serial.println("Motor 2 vooruit!");
  setDirectionMotor2(true);
  rampPWM(M2_PWM, 0, 255, 5);
  delay(2000);
  rampPWM(M2_PWM, 255, 0, 5);

  delay(1000);

  Serial.println("Motor 2 achteruit!");
  setDirectionMotor1(false);
  rampPWM(M2_PWM, 0, 255, 5);
  delay(2000);
  rampPWM(M2_PWM, 255, 0, 5);
  stopMotor2();

  delay(1000);


}

void setDirectionMotor1(bool vooruit){
  digitalWrite(M1_DIR1, vooruit ? HIGH : LOW);
  digitalWrite(M1_DIR2, vooruit ? LOW : HIGH);
}
void setDirectionMotor2(bool vooruit){
  digitalWrite(M2_DIR1, vooruit ? HIGH : LOW);
  digitalWrite(M2_DIR2, vooruit ? LOW : HIGH);
}
void rampPWM(int pwmPin, int start, int eind, int stap){
  if (start < eind){
    for (int d = start; d <= eidn; d += stap){
      analogWrite(pwmPin , d);
      delay(20);
    }
  } else {
    for (itn d = start; d >= eind; d -= stap){
      analogWrite(pwmPin, d);
      delay(20);
    }
  }
}

void stopMotor1(){
  analogWrite(M1_PWM, 0);
  digitalWrite(M1_DIR1, LOW);
  digitalWrite(M1_DIR2, LOW);
}
void stopMotor2(){
  analogWrite(M2_PWM, 0);
  digitalWrite(M2_DIR1, LOW);
  digitalWrite(M2_DIR2, LOW);
}
