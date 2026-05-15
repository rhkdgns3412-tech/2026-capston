#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MPU6050.h>
#include "MAX30105.h"

// ================= 핀 설정 =================
#define SDA 21
#define SCL 22

// ================= 센서 객체 =================
Adafruit_BME280 bme;
MPU6050 mpu;
MAX30105 max30102;

// ================= 보정값 =================
#define BME_TEMP_OFFSET   0.0
#define BME_HUM_OFFSET    0.0
#define BME_PRESS_OFFSET  0.0

int16_t axOffset = 0, ayOffset = 0, azOffset = 0;
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;

long irBase = 0;
long redBase = 0;

// ================= 측정값 =================
float temperature, humidity, pressure;
int16_t ax, ay, az, gx, gy, gz;
long irValue, redValue;
bool fingerDetected = false;

// ================= 타이머 =================
unsigned long previousMillis = 0;
#define MEASURE_INTERVAL 1000

// ================= 함수 선언 =================
void initBME280();
void initMPU6050();
void initMAX30102();

void calibrateMPU6050();
void calibrateMAX30102();

void readBME280();
void readMPU6050();
void readMAX30102();

void printSensorData();

// ================= setup =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA, SCL);

  initBME280();
  initMPU6050();
  initMAX30102();

  Serial.println("전체 센서 초기화 완료");
}

// ================= loop =================
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= MEASURE_INTERVAL) {
    previousMillis = currentMillis;

    readBME280();
    readMPU6050();
    readMAX30102();

    printSensorData();
  }
}

// ================= 함수 정의 =================

// ---------- 초기화 ----------
void initBME280() {
  if (!bme.begin(0x76)) {
    Serial.println("BME280 연결 실패");
    while (1);
  }
  Serial.println("BME280 시작");
}

void initMPU6050() {
  mpu.initialize();



  Serial.println("MPU6050 시작");
  calibrateMPU6050();
}

void initMAX30102() {
  if (!max30102.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 연결 실패");
    while (1);
  }

  max30102.setup();
  Serial.println("MAX30102 시작");

  calibrateMAX30102();
}

// ---------- 보정 ----------
void calibrateMPU6050() {
  long axSum = 0, aySum = 0, azSum = 0;
  long gxSum = 0, gySum = 0, gzSum = 0;

  for (int i = 0; i < 100; i++) {
    int16_t rax, ray, raz, rgx, rgy, rgz;
    mpu.getMotion6(&rax, &ray, &raz, &rgx, &rgy, &rgz);

    axSum += rax;
    aySum += ray;
    azSum += raz;
    gxSum += rgx;
    gySum += rgy;
    gzSum += rgz;

    delay(10);
  }

  axOffset = axSum / 100;
  ayOffset = aySum / 100;
  azOffset = (azSum / 100) - 16384;

  gxOffset = gxSum / 100;
  gyOffset = gySum / 100;
  gzOffset = gzSum / 100;

  Serial.println("MPU6050 보정 완료");
}

void calibrateMAX30102() {
  long irSum = 0, redSum = 0;

  for (int i = 0; i < 50; i++) {
    irSum += max30102.getIR();
    redSum += max30102.getRed();
    delay(20);
  }

  irBase = irSum / 50;
  redBase = redSum / 50;

  Serial.println("MAX30102 기준값 설정 완료");
}

// ---------- 센서 읽기 ----------
void readBME280() {
  temperature = bme.readTemperature() + BME_TEMP_OFFSET;
  humidity = bme.readHumidity() + BME_HUM_OFFSET;
  pressure = (bme.readPressure() / 100.0) + BME_PRESS_OFFSET;
}

void readMPU6050() {
  int16_t rax, ray, raz, rgx, rgy, rgz;
  mpu.getMotion6(&rax, &ray, &raz, &rgx, &rgy, &rgz);

  ax = rax - axOffset;
  ay = ray - ayOffset;
  az = raz - azOffset;

  gx = rgx - gxOffset;
  gy = rgy - gyOffset;
  gz = rgz - gzOffset;
}

void readMAX30102() {
  irValue = max30102.getIR();
  redValue = max30102.getRed();

  fingerDetected = (irValue > 50000);
}

// ---------- 출력 ----------
void printSensorData() {
  Serial.println("================================");

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | Hum: ");
  Serial.print(humidity);
  Serial.print(" % | Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  Serial.print("ACC: ");
  Serial.print(ax); Serial.print(", ");
  Serial.print(ay); Serial.print(", ");
  Serial.print(az);

  Serial.print(" | GYRO: ");
  Serial.print(gx); Serial.print(", ");
  Serial.print(gy); Serial.print(", ");
  Serial.println(gz);

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" | RED: ");
  Serial.print(redValue);

  if (fingerDetected) Serial.println(" | 접촉 OK");
  else Serial.println(" | 접촉 불량");
}