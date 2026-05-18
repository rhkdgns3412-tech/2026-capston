#include "sensors.h"

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MPU6050.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <BH1750.h>

static const uint8_t MAX30205_ADDR = 0x48;
static const uint8_t MAX30205_TEMP_REG = 0x00;

Adafruit_BME280 bme;
MPU6050 mpu;
MAX30105 max30102;
BH1750 lightMeter;

bool bmeReady = false;
bool mpuReady = false;
bool maxReady = false;
bool bhReady = false;
bool max30205Ready = false;

static const uint16_t MAX30102_SAMPLE_INTERVAL_MS = 40;
static const uint16_t MAX30102_RECALC_SAMPLES = 25;

static uint32_t irBuffer[BUFFER_SIZE];
static uint32_t redBuffer[BUFFER_SIZE];
static uint16_t max30102BufferIndex = 0;
static uint16_t max30102SamplesFilled = 0;
static uint16_t max30102SamplesSinceCalc = 0;
static unsigned long lastMAX30102SampleMillis = 0;
static int32_t max30102HeartRate = 0;
static int32_t max30102SpO2 = 0;
static int8_t max30102ValidHeartRate = 0;
static int8_t max30102ValidSpO2 = 0;

#define BME_TEMP_OFFSET   0.0
#define BME_HUM_OFFSET    0.0
#define BME_PRESS_OFFSET  0.0

int16_t axOffset = 0, ayOffset = 0, azOffset = 0;
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;

long irBase = 0;
long redBase = 0;

void initBME280();
void initMPU6050();
void initMAX30102();
void initBH1750();
void initMAX30205();

void calibrateMPU6050();
void calibrateMAX30102();

void readBME280(SensorData &data);
void readMPU6050(SensorData &data);
void readMAX30102(SensorData &data);
void readBH1750(SensorData &data);
void readMAX30205(SensorData &data);

void initSensors() {
  Wire.begin(SDA, SCL);
  // 40cm 긴 점퍼선 사용 시 낮은 속도 필요
  Wire.setClock(50000);  // 100kHz -> 50kHz로 낮춤
  delay(500);
  
  Serial.println("\n=== 센서 초기화 시작 ===");
  Serial.println("주의: 40cm 점퍼선 사용 중 - Pull-up 저항 4.7kΩ 확인 필요");

  initBME280();
  delay(300);

  initMPU6050();
  delay(300);

  initMAX30102();
  delay(300);
  
  initMAX30205();
  delay(300);
  
  initBH1750();
  delay(300);
  
  Serial.println("=== 센서 초기화 완료 ===");
}

void readSensors(SensorData &data) {
  readBME280(data);


  readMPU6050(data);


  readMAX30102(data);
  readMAX30205(data);
  readBH1750(data);
}

void initBME280() {
  if (bme.begin(0x76)) {
    bmeReady = true;
    Serial.println("BME280 시작: 주소 0x76");
    return;
  }

  if (bme.begin(0x77)) {
    bmeReady = true;
    Serial.println("BME280 시작: 주소 0x77");
    return;
  }

  bmeReady = false;
  Serial.println("BME280 연결 실패: 0x76 / 0x77 모두 실패");
}

void initMPU6050() {
  mpu.initialize();
  mpuReady = true;
  Serial.println("MPU6050 시작");
  calibrateMPU6050();
}

void initMAX30102() {
  // 재연결 시도 (최대 3회)
  for (int attempt = 0; attempt < 3; attempt++) {
    if (max30102.begin(Wire, I2C_SPEED_STANDARD)) {
      max30102.setup(60, 4, 2, 100, 411, 4096);
      max30102.clearFIFO();
      maxReady = true;
      Serial.print("MAX30102 시작 (시도 ");
      Serial.print(attempt + 1);
      Serial.println("/3)");
      calibrateMAX30102();
      return;
    }
    delay(200);
  }
  
  maxReady = false;
  Serial.println("MAX30102 연결 실패 - I2C 케이블 연결 확인 필요");
}

void initBH1750() {
  // 재연결 시도 (최대 3회)
  for (int attempt = 0; attempt < 3; attempt++) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      bhReady = true;
      Serial.print("BH1750 시작 (시도 ");
      Serial.print(attempt + 1);
      Serial.println("/3)");
      return;
    }
    delay(200);
  }
  
  bhReady = false;
  Serial.println("BH1750 연결 실패 - ADDR 핀 설정 확인 필요 (0x23 또는 0x5C)");
}

void initMAX30205() {
  // 재연결 시도 (최대 3회)
  for (int attempt = 0; attempt < 3; attempt++) {
    delay(150);
    
    Wire.beginTransmission(MAX30205_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
      // I2C 주소 확인됨, 레지스터 읽기 시도
      delay(50);
      Wire.beginTransmission(MAX30205_ADDR);
      Wire.write(0x00); // Temperature register
      if (Wire.endTransmission() != 0) {
        continue; // 다시 시도
      }

      Wire.requestFrom((int)MAX30205_ADDR, 2);
      if (Wire.available() >= 2) {
        uint8_t test_msb = Wire.read();
        uint8_t test_lsb = Wire.read();
        max30205Ready = true;
        Serial.print("MAX30205 시작 (시도 ");
        Serial.print(attempt + 1);
        Serial.print("/3): 초기값 ");
        Serial.print(test_msb); Serial.print(" ");
        Serial.println(test_lsb);
        return;
      }
    } else {
      Serial.print("MAX30205 재시도 ");
      Serial.print(attempt + 1);
      Serial.print("/3 - 에러코드: ");
      Serial.println(error);
    }
  }
  
  max30205Ready = false;
  Serial.println("MAX30205 연결 실패 - I2C 주소(0x48) 확인 필요");
}

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
  if (!maxReady) return;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
  }

  max30102BufferIndex = 0;
  max30102SamplesFilled = 0;
  max30102SamplesSinceCalc = 0;
  lastMAX30102SampleMillis = millis();
  max30102HeartRate = 0;
  max30102SpO2 = 0;
  max30102ValidHeartRate = 0;
  max30102ValidSpO2 = 0;

  Serial.println("MAX30102 버퍼 초기화 완료");
}

void readBME280(SensorData &data) {
  if (!bmeReady) {
    data.temperature = 0;
    data.humidity = 0;
    data.pressure = 0;
    return;
  }

  data.temperature = bme.readTemperature() + BME_TEMP_OFFSET;
  data.humidity = bme.readHumidity() + BME_HUM_OFFSET;
  data.pressure = (bme.readPressure() / 100.0) + BME_PRESS_OFFSET;
}

void readMPU6050(SensorData &data) {
  if (!mpuReady) return;

  int16_t rax, ray, raz, rgx, rgy, rgz;
  mpu.getMotion6(&rax, &ray, &raz, &rgx, &rgy, &rgz);

  data.ax = rax - axOffset;
  data.ay = ray - ayOffset;
  data.az = raz - azOffset;

  data.gx = rgx - gxOffset;
  data.gy = rgy - gyOffset;
  data.gz = rgz - gzOffset;
}

void readMAX30102(SensorData &data) {
  if (!maxReady) {
    data.irValue = 0;
    data.redValue = 0;
    data.fingerDetected = false;
    data.heartRate = 0;
    data.spo2 = 0;
    return;
  }

  // 간단한 샘플 읽기 (테스트 코드 방식)
  if (max30102.available() > 0) {
    uint32_t irSample = max30102.getIR();
    uint32_t redSample = max30102.getRed();
    max30102.nextSample();

    redBuffer[max30102BufferIndex] = redSample;
    irBuffer[max30102BufferIndex] = irSample;

    max30102BufferIndex++;
    if (max30102BufferIndex >= BUFFER_SIZE) {
      max30102BufferIndex = 0;
    }

    if (max30102SamplesFilled < BUFFER_SIZE) {
      max30102SamplesFilled++;
    }

    if (max30102SamplesSinceCalc < MAX30102_RECALC_SAMPLES) {
      max30102SamplesSinceCalc++;
    }
  } else {
    max30102.check();
  }

  // 최신 데이터 반환
  uint16_t lastIndex = (max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE;
  data.irValue = irBuffer[lastIndex];
  data.redValue = redBuffer[lastIndex];
  data.fingerDetected = (data.irValue > 50000);

  // 버퍼가 충분히 찼을 때만 SpO2/심박 계산
  if (max30102SamplesFilled >= BUFFER_SIZE) {
    if (max30102SamplesSinceCalc >= MAX30102_RECALC_SAMPLES) {
      uint32_t orderedIR[BUFFER_SIZE];
      uint32_t orderedRed[BUFFER_SIZE];

      for (int i = 0; i < BUFFER_SIZE; i++) {
        uint16_t index = (max30102BufferIndex + i) % BUFFER_SIZE;
        orderedIR[i] = irBuffer[index];
        orderedRed[i] = redBuffer[index];
      }

      maxim_heart_rate_and_oxygen_saturation(
        orderedIR,
        BUFFER_SIZE,
        orderedRed,
        &max30102SpO2,
        &max30102ValidSpO2,
        &max30102HeartRate,
        &max30102ValidHeartRate
      );

      max30102SamplesSinceCalc = 0;
    }
  }

  if (max30102ValidHeartRate && data.fingerDetected) {
    data.heartRate = max30102HeartRate;
  } else {
    data.heartRate = 0;
  }

  if (max30102ValidSpO2 && data.fingerDetected) {
    data.spo2 = max30102SpO2;
  } else {
    data.spo2 = 0;
  }
}

void readBH1750(SensorData &data) {
  if (!bhReady) {
    data.lux = 0;
    return;
  }

  data.lux = lightMeter.readLightLevel();
}

void readMAX30205(SensorData &data) {
  if (!max30205Ready) {
    data.bodyTemp = 0;
    return;
  }

  // MAX30205 온도 레지스터 읽기
  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(0x00); // Temperature register
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) {
    data.bodyTemp = 0;
    return;
  }

  // 2바이트 데이터 요청
  int bytesRead = Wire.requestFrom((int)MAX30205_ADDR, 2);
  if (bytesRead < 2) {
    data.bodyTemp = 0;
    return;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  
  // 온도 데이터: MSB는 정수부, LSB는 소수부 (1/256 단위)
  // 예: 37.5°C => MSB=37(0x25), LSB=128(0x80)
  int16_t rawTemp = (int16_t)((msb << 8) | lsb);
  
  // 오른쪽으로 4비트 시프트 (하위 4비트만 온도 데이터)
  data.bodyTemp = (rawTemp >> 4) * 0.0625f;
}

void updateDerivedData(SensorData &data) {
  if (data.fingerDetected) {
    if (data.heartRate < 0) {
      data.heartRate = 0;
    }
    if (data.spo2 < 0) {
      data.spo2 = 0;
    }
  } else {
    data.heartRate = 0;
    data.spo2 = 0;
  }

  long accPower = abs(data.ax) + abs(data.ay) + abs(data.az);

  if (accPower > 30000) {
    data.posture = "UNSTABLE";
  } else {
    data.posture = "NORMAL";
  }
}

void printSensorData(const SensorData &data) {
  Serial.println("================================");

  Serial.print("Env Temp: ");
  Serial.print(data.temperature);
  Serial.print(" C | Hum: ");
  Serial.print(data.humidity);
  Serial.print(" % | Pressure: ");
  Serial.print(data.pressure);
  Serial.println(" hPa");

  Serial.print("ACC: ");
  Serial.print(data.ax); Serial.print(", ");
  Serial.print(data.ay); Serial.print(", ");
  Serial.print(data.az);

  Serial.print(" | GYRO: ");
  Serial.print(data.gx); Serial.print(", ");
  Serial.print(data.gy); Serial.print(", ");
  Serial.println(data.gz);

  Serial.print("IR: ");
  Serial.print(data.irValue);
  Serial.print(" | RED: ");
  Serial.print(data.redValue);

  if (data.fingerDetected) Serial.println(" | 접촉 OK");
  else Serial.println(" | 접촉 불량");

  Serial.print("Light: ");
  Serial.print(data.lux);
  Serial.println(" lx");

  Serial.print("Body Temp: ");
  Serial.print(data.bodyTemp);
  Serial.println(" C");
}