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
  Wire.setClock(50000);
  delay(500);

  initBME280();

  initMPU6050();


  initMAX30102();
  initMAX30205();
  initBH1750();
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
  if (!max30102.begin(Wire, I2C_SPEED_STANDARD)) {
    maxReady = false;
    Serial.println("MAX30102 연결 실패");
    return;
  }

  max30102.setup(60, 4, 2, 100, 411, 4096);
  max30102.clearFIFO();
  maxReady = true;

  Serial.println("MAX30102 시작");
  calibrateMAX30102();
}

void initBH1750() {
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    bhReady = false;
    Serial.println("BH1750 연결 실패");
    return;
  }

  bhReady = true;
  Serial.println("BH1750 시작");
}

void initMAX30205() {
  Wire.beginTransmission(MAX30205_ADDR);
  if (Wire.endTransmission() != 0) {
    max30205Ready = false;
    Serial.println("MAX30205 연결 실패");
    return;
  }

  max30205Ready = true;
  Serial.println("MAX30205 시작");
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

  unsigned long currentMillis = millis();

  while ((currentMillis - lastMAX30102SampleMillis) >= MAX30102_SAMPLE_INTERVAL_MS) {
    while (max30102.available() == false) {
      max30102.check();
    }

    uint32_t redSample = max30102.getRed();
    uint32_t irSample = max30102.getIR();
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

    lastMAX30102SampleMillis += MAX30102_SAMPLE_INTERVAL_MS;
    currentMillis = millis();
  }

  if (max30102SamplesFilled < BUFFER_SIZE) {
    data.irValue = irBuffer[(max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE];
    data.redValue = redBuffer[(max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE];
    data.fingerDetected = (data.irValue > 50000);
    data.heartRate = 0;
    data.spo2 = 0;
    return;
  }

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

  data.irValue = irBuffer[(max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE];
  data.redValue = redBuffer[(max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE];
  data.fingerDetected = (data.irValue > 50000);

  if (max30102ValidHeartRate) {
    data.heartRate = max30102HeartRate;
  } else {
    data.heartRate = 0;
  }

  if (max30102ValidSpO2) {
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

  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(MAX30205_TEMP_REG);
  if (Wire.endTransmission() != 0) {
    data.bodyTemp = 0;
    return;
  }

  if (Wire.requestFrom((int)MAX30205_ADDR, 2) < 2) {
    data.bodyTemp = 0;
    return;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  int16_t rawTemp = (int16_t)((msb << 8) | lsb);

  data.bodyTemp = rawTemp * 0.00390625f;
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