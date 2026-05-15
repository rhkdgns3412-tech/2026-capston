/*#include <Wire.h>
#include <MPU6050.h>

#define SDA 21
#define SCL 22

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);

  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 연결 실패");
    while (1);
  }

  Serial.println("MPU6050 시작");
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  Serial.print("ACC: ");
  Serial.print(ax); Serial.print(", ");
  Serial.print(ay); Serial.print(", ");
  Serial.print(az);

  Serial.print(" | GYRO: ");
  Serial.print(gx); Serial.print(", ");
  Serial.print(gy); Serial.print(", ");
  Serial.println(gz);

  delay(500);
}
*/

#include <Wire.h>

#define SDA 21
#define SCL 22

void setup() {
  Wire.begin(SDA, SCL);

  Serial.begin(115200);
  delay(2000);

  Serial.println("I2C Scanner 시작");
}

void loop() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");

      if (address < 16)
        Serial.print("0");

      Serial.println(address, HEX);

      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("I2C 장치 없음");
  }
  else {
    Serial.println("Scan 완료");
  }

  Serial.println();

  delay(3000);
}