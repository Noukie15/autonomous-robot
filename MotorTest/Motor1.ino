#define PIN_M1 2
#define PIN_M2 3
#define PIN_M3_PWM 4

const int pwmFreq = 20000;
const int pwmResolution = 8;
int duty = 0;

void setup(){
  pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M2, OUTPUT);
  pinMode(PIN_M3_PWM, OUTPUT);

  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, LOW);
  digitalWrite(PIN_M3_PWM, LOW);

  Serial.begin();
  delay(3000);
  Serial.println("H-brug test gestart!!")
}

void loop(){
  Serial.println("Richting 1 - versnellen!")
  digitalWrite(PIN_M1, HIGH);
  digitalWrite(PIN_M2, LOW);
  for(duty = 0; duty <= 255; duty++){
    analogWrite(PIN_M3_PWM, duty);
    delay(50);
  }

  delay(1000);

  Serial.prinln("Richting 1 - vertragen!");
  for(duty = 255; duty >= 0; duty--){
    analogWrite(PIN_M3_PWM, duty);
    delay(50);
  }

  delay(1000);
  Serial.println("Richting 2 - versnellen!");
  digitalWrite(PIN_M1, LOW);
  digitalWrite(PIN_M2, HIGH);
  for(duty = 0; duty <= 255; duty++){
    analogWrite(PIN_M3_PWM, duty);
    delay(50);
  }

  delay(1000);

  Serial.prinln("Richting 2 - vertragen!");
  for(duty = 255; duty >= 0; duty--){
    analogWrite(PIN_M3_PWM, duty);
    delay(50);
  }

  delay(1000);
}
