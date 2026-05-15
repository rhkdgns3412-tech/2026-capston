#include <Wire.h>

#define SDA 21
#define SCL 22

#define MAX30205_ADDR 0x48

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);

  // MAX30205 연결 확인
  Wire.beginTransmission(MAX30205_ADDR);

  if (Wire.endTransmission() != 0) {
    Serial.println("MAX30205 연결 실패");
    while (1);
  }

  Serial.println("MAX30205 시작");
}

float readBodyTemp() {
  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(0x00); // Temperature register
  Wire.endTransmission();

  Wire.requestFrom(MAX30205_ADDR, 2);

  if (Wire.available() < 2) {
    return -999;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t rawTemp = (msb << 8) | lsb;

  return rawTemp * 0.00390625;
}

void loop() {
  float bodyTemp = readBodyTemp();

  Serial.print("Body Temp: ");
  Serial.print(bodyTemp);
  Serial.println(" °C");

  delay(1000);
}