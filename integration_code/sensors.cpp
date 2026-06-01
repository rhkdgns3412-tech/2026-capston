#include "sensors.h"

#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MPU6050.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <BH1750.h>
#include "spo2_algorithm.h"
static const uint8_t MAX30205_ADDR    = 0x48;
static const uint8_t MAX30205_TEMP_REG = 0x00;
static int hrHistory[5] = {60, 60, 60, 60, 60};
static int hrIdx = 0;
Adafruit_BME280 bme;
MPU6050 mpu;
MAX30105 max30102;
BH1750 lightMeter;

bool bmeReady      = false;
bool mpuReady      = false;
bool maxReady      = false;
bool bhReady       = false;
bool max30205Ready = false;

static const uint16_t MAX30102_MIN_VALID_SAMPLES = BUFFER_SIZE;

static uint32_t irBuffer[BUFFER_SIZE];
static uint32_t redBuffer[BUFFER_SIZE];
static uint16_t max30102BufferIndex    = 0;
static uint16_t max30102SamplesFilled  = 0;
static uint16_t samplesSinceCalc       = 0;  // [추가] 재계산 주기용
static unsigned long lastMAX30102SampleMillis = 0;
static int32_t  max30102HeartRate      = 0;
static int32_t  max30102SpO2           = 0;
static int8_t   max30102ValidHeartRate = 0;
static int8_t   max30102ValidSpO2      = 0;

#define BME_TEMP_OFFSET   -3.0f
#define BME_HUM_OFFSET    20.0f
#define BME_PRESS_OFFSET   0.0f

#define MAX30205_TEMP_OFFSET 1.7f

int16_t axOffset = 0, ayOffset = 0, azOffset = 0;
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;

// 전방 선언
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

// ================================================================
// 초기화
// ================================================================

void initSensors() {
  Wire.begin(SDA, SCL);
  Wire.setClock(50000);
  delay(500);

  Serial.println("\n=== 센서 초기화 시작 ===");

  initBME280();   delay(300);
  initMPU6050();  delay(300);
  initMAX30102(); delay(300);
  initMAX30205(); delay(300);
  initBH1750();   delay(300);

  Serial.println("=== 센서 초기화 완료 ===");
}

void readSensors(SensorData &data) {
  readBME280(data);
  readMPU6050(data);
  readMAX30102(data);
  readMAX30205(data);
  readBH1750(data);
}

// ================================================================
// 개별 센서 초기화
// ================================================================

void initBME280() {
  if (bme.begin(0x76)) { bmeReady = true; Serial.println("BME280 시작: 주소 0x76"); return; }
  if (bme.begin(0x77)) { bmeReady = true; Serial.println("BME280 시작: 주소 0x77"); return; }
  Serial.println("BME280 연결 실패: 0x76 / 0x77 모두 실패");
}

void initMPU6050() {
  mpu.initialize();
  mpuReady = true;
  Serial.println("MPU6050 시작");
  calibrateMPU6050();
}


void initMAX30102() {
  for (int attempt = 0; attempt < 3; attempt++) {
    if (max30102.begin(Wire, I2C_SPEED_STANDARD)) {
      max30102.setup(15, 8, 2, 25, 411, 4096);
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
  for (int attempt = 0; attempt < 3; attempt++) {
    delay(150);
    Wire.beginTransmission(MAX30205_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      delay(50);
      Wire.beginTransmission(MAX30205_ADDR);
      Wire.write(MAX30205_TEMP_REG);
      if (Wire.endTransmission() != 0) continue;

      Wire.requestFrom((int)MAX30205_ADDR, 2);
      if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        max30205Ready = true;
        Serial.print("MAX30205 시작 (시도 ");
        Serial.print(attempt + 1);
        Serial.print("/3): 초기값 MSB=");
        Serial.print(msb);
        Serial.print(" LSB=");
        Serial.println(lsb);
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

// ================================================================
// 캘리브레이션
// ================================================================

void calibrateMPU6050() {
  long axS=0, ayS=0, azS=0, gxS=0, gyS=0, gzS=0;
  for (int i = 0; i < 100; i++) {
    int16_t rax, ray, raz, rgx, rgy, rgz;
    mpu.getMotion6(&rax, &ray, &raz, &rgx, &rgy, &rgz);
    axS+=rax; ayS+=ray; azS+=raz;
    gxS+=rgx; gyS+=rgy; gzS+=rgz;
    delay(10);
  }
  axOffset = axS/100; ayOffset = ayS/100;
  azOffset = (azS/100) - 16384;
  gxOffset = gxS/100; gyOffset = gyS/100; gzOffset = gzS/100;
  Serial.println("MPU6050 보정 완료");
}

void calibrateMAX30102() {
  if (!maxReady) return;
  memset(irBuffer,  0, sizeof(irBuffer));
  memset(redBuffer, 0, sizeof(redBuffer));
  max30102BufferIndex    = 0;
  max30102SamplesFilled  = 0;
  samplesSinceCalc       = 0;
  lastMAX30102SampleMillis = millis();
  max30102HeartRate      = 0;
  max30102SpO2           = 0;
  max30102ValidHeartRate = 0;
  max30102ValidSpO2      = 0;
  Serial.println("MAX30102 버퍼 초기화 완료");
}

// ================================================================
// 센서 읽기
// ================================================================

void readBME280(SensorData &data) {
  if (!bmeReady) { data.temperature=0; data.humidity=0; data.pressure=0; return; }
  data.temperature = bme.readTemperature() + BME_TEMP_OFFSET;
  data.humidity    = bme.readHumidity()    + BME_HUM_OFFSET;
  data.humidity    = constrain(data.humidity, 0.0f, 100.0f);
  data.pressure    = (bme.readPressure() / 100.0f) + BME_PRESS_OFFSET;
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
static void computeHRandSpO2(const uint32_t *ir, const uint32_t *red, int len,
                              int32_t *hr, int32_t *spo2,
                              int8_t *validHR, int8_t *validSpO2) {
  if (!ir || !red || len <= 0) return;

  maxim_heart_rate_and_oxygen_saturation(
    const_cast<uint32_t *>(ir), len,
    const_cast<uint32_t *>(red),
    spo2, validSpO2,
    hr, validHR
  );
}
void readMAX30102(SensorData &data) {
  if (max30102ValidHeartRate &&
    max30102HeartRate >= 40 &&
    max30102HeartRate <= 180) {
    hrHistory[hrIdx] = max30102HeartRate;
    hrIdx = (hrIdx + 1) % 5;

    int sum = 0;
    for (int i = 0; i < 5; i++) sum += hrHistory[i];
    data.heartRate = sum / 5;
}

  // [수정 1] if → while: FIFO에 쌓인 샘플 전부 소비
  // 25Hz 설정 + 500ms 루프 = 최대 12~13개 샘플 누적
  // 1개만 읽으면 FIFO 오버플로우 → HR 계산 오염
  while (max30102.available() > 0) {
    uint32_t irSample  = max30102.getIR();
    uint32_t redSample = max30102.getRed();
    max30102.nextSample();

    irBuffer[max30102BufferIndex]  = irSample;
    redBuffer[max30102BufferIndex] = redSample;
    max30102BufferIndex = (max30102BufferIndex + 1) % BUFFER_SIZE;

    if (max30102SamplesFilled < BUFFER_SIZE) max30102SamplesFilled++;
    samplesSinceCalc++;
    if (max30102ValidHeartRate && 
    max30102HeartRate >= 40 && 
    max30102HeartRate <= 180) {
    data.heartRate = max30102HeartRate;
}
  }
  // [수정] check()는 while 이후에만 호출
  max30102.check();

  // 최신 샘플 인덱스
  uint16_t lastIdx = (max30102BufferIndex + BUFFER_SIZE - 1) % BUFFER_SIZE;
  data.irValue  = irBuffer[lastIdx];
  data.redValue = redBuffer[lastIdx];

  // [수정 2] 손목 착용 기준 fingerDetected 범위 수정
  // 기존: > 50000 (손목에서 접촉 인식 안 되는 경우 많음)
  // 변경: 20000 ~ 200000
  data.fingerDetected = (data.irValue > 20000 && data.irValue < 200000);

  // [수정 3] 25샘플마다 1번만 재계산
  // 기존: 버퍼 찰 때마다 매번 100샘플 재처리 → 연산 낭비
  if (max30102SamplesFilled >= BUFFER_SIZE && samplesSinceCalc >= 25) {
    uint32_t ordIR[BUFFER_SIZE];
    uint32_t ordRed[BUFFER_SIZE];

    for (int i = 0; i < BUFFER_SIZE; i++) {
      uint16_t idx = (max30102BufferIndex + i) % BUFFER_SIZE;
      ordIR[i]  = irBuffer[idx];
      ordRed[i] = redBuffer[idx];
    }

    computeHRandSpO2(ordIR, ordRed, BUFFER_SIZE,
                     &max30102HeartRate, &max30102SpO2,
                     &max30102ValidHeartRate, &max30102ValidSpO2);
    samplesSinceCalc = 0;
  }

  // validHR/validSpO2 플래그만 확인, fingerDetected로 강제 0 안 함
  data.heartRate = max30102ValidHeartRate ? max30102HeartRate : data.heartRate;
  data.heartRate=(data.heartRate);
  data.spo2      = (max30102ValidSpO2 && data.fingerDetected)
                    ? max30102SpO2 : data.spo2;
}

void readBH1750(SensorData &data) {
  if (!bhReady) { data.lux = 0; return; }
  data.lux = lightMeter.readLightLevel();
}

void readMAX30205(SensorData &data) {
  if (!max30205Ready) { data.bodyTemp = 0; return; }

  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(MAX30205_TEMP_REG);
  if (Wire.endTransmission() != 0) { data.bodyTemp = 0; return; }

  if (Wire.requestFrom((int)MAX30205_ADDR, 2) < 2) { data.bodyTemp = 0; return; }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t rawTemp = (int16_t)((msb << 8) | lsb);
  data.bodyTemp = (rawTemp / 256.0f) + MAX30205_TEMP_OFFSET;
}

// ================================================================
// 파생 데이터 업데이트
// ================================================================

void updateDerivedData(SensorData &data) {
  // SpO2만 fingerDetected 조건 유지 (접촉 없으면 SpO2 신뢰 불가)
  // HR은 fingerDetected와 무관하게 유지
  if (!data.fingerDetected) {
    data.spo2 = 50;      
    data.heartRate = 30;
  }

  long accPower = abs(data.ax) + abs(data.ay) + abs(data.az);
  data.posture = (accPower > 30000) ? "UNSTABLE" : "NORMAL";
}

// ================================================================
// Serial 출력
// ================================================================

void printSensorData(const SensorData &data) {
  Serial.println("================================");

  Serial.print("Env Temp: ");  Serial.print(data.temperature);
  Serial.print(" C | Hum: "); Serial.print(data.humidity);
  Serial.print(" % | Press: "); Serial.print(data.pressure);
  Serial.println(" hPa");

  Serial.print("ACC: ");
  Serial.print(data.ax); Serial.print(", ");
  Serial.print(data.ay); Serial.print(", ");
  Serial.print(data.az);
  Serial.print(" | GYRO: ");
  Serial.print(data.gx); Serial.print(", ");
  Serial.print(data.gy); Serial.print(", ");
  Serial.println(data.gz);

  Serial.print("IR: ");   Serial.print(data.irValue);
  Serial.print(" | RED: "); Serial.print(data.redValue);
  Serial.print(" | HR: ");  Serial.print(data.heartRate);
  Serial.print(" | SpO2: "); Serial.print(data.spo2);
  if (data.fingerDetected) Serial.println(" | 접촉 OK");
  else                     Serial.println(" | 접촉 불량");

  Serial.print("Body Temp: "); Serial.print(data.bodyTemp); Serial.println(" C");
  Serial.print("Light: ");     Serial.print(data.lux);      Serial.println(" lx");
  Serial.print("Posture: ");   Serial.println(data.posture);
}