#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

#define SDA 21
#define SCL 22
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA , SCL);

  Serial.println("MAX30102 테스트 시작");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("센서 연결 실패");
    while (1);
  }

  // 기본 설정 (테스트용)
  particleSensor.setup();  
                             
  Serial.println("센서 연결 성공");
}

void loop() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" | RED: ");
  Serial.println(redValue);

  delay(200);
}