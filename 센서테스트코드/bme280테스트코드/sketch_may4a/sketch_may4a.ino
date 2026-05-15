#include <Wire.h>
#include <Adafruit_BME280.h>

#define SDA 21
#define SCL 22

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  

  if (!bme.begin(0x76)) {  // 0x76 또는 0x77
    Serial.println("BME280 연결 실패");
    while (1);
  }

  Serial.println("BME280 시작");
}

void loop() {
  Serial.print("Temp: ");
  Serial.print(bme.readTemperature());
  Serial.print(" °C | Hum: ");
  Serial.print(bme.readHumidity());
  Serial.print(" % | Pressure: ");
  Serial.print(bme.readPressure() / 100.0);
  Serial.println(" hPa");

  delay(1000);
}