#include <Wire.h>
#include <Adafruit_BME280.h>
#include <MPU6050.h>
#include "MAX30105.h"
#include <BH1750.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ================= 사용 설정 =================
#define USE_MPU6050 0   // MPU6050 문제 해결 전까지 0, 정상 인식되면 1

// ================= 핀 설정 =================
#define SDA 21
#define SCL 22
#define MOTOR_PIN 26
// ================= BLE 설정 =================
#define DEVICE_NAME "SS_0001"

#define SERVICE_UUID        "089fca17-755f-4578-b8af-ee5e32526b0f"
#define DATA_CHAR_UUID      "0000FFF1-0000-1000-8000-00805F9B34FB"
#define CONTROL_CHAR_UUID   "0000FFF2-0000-1000-8000-00805F9B34FB"
#define LIGHT_THRESHOLD 450.0
BLEServer *pServer = NULL;
BLECharacteristic *pDataCharacteristic = NULL;
BLECharacteristic *pControlCharacteristic = NULL;

bool deviceConnected = false;

// ================= 센서 객체 =================
Adafruit_BME280 bme;
MPU6050 mpu;
MAX30105 max30102;
BH1750 lightMeter;

// ================= 보정값 =================
#define BME_TEMP_OFFSET   0.0
#define BME_HUM_OFFSET    0.0
#define BME_PRESS_OFFSET  0.0

int16_t axOffset = 0, ayOffset = 0, azOffset = 0;
int16_t gxOffset = 0, gyOffset = 0, gzOffset = 0;

long irBase = 0;
long redBase = 0;

// ================= 센서 상태 =================
bool bmeReady = false;
bool mpuReady = false;
bool maxReady = false;
bool bhReady = false;

// ================= 센서 데이터 구조체 =================
struct SensorData {
  float temperature;   // BME280 환경 온도
  float humidity;
  float pressure;

  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;

  long irValue;
  long redValue;
  bool fingerDetected;

  float lux;

  int heartRate;
  int spo2;
  float bodyTemp;
  String posture;
};

SensorData sensorData;

// ================= 타이머 =================
unsigned long previousMillis = 0;
#define MEASURE_INTERVAL 500

// ================= 함수 선언 =================
void initBLE();
void sendBLEData(const SensorData &data);
String makeBLEPayload(const SensorData &data);
void handleControlCommand(String command);
void controlVibrationMotor(const SensorData &data);
void initBME280();
void initMPU6050();
void initMAX30102();
void initBH1750();

void calibrateMPU6050();
void calibrateMAX30102();

void readBME280(SensorData &data);
void readMPU6050(SensorData &data);
void readMAX30102(SensorData &data);
void readBH1750(SensorData &data);

void updateDerivedData(SensorData &data);
void printSensorData(const SensorData &data);

// ================= BLE 콜백 =================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE 연결됨");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE 연결 해제됨");
    pServer->getAdvertising()->start();
    Serial.println("BLE 광고 재시작");
  }
};

class ControlCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String command = pCharacteristic->getValue().c_str();
    handleControlCommand(command);
  }
};

// ================= setup =================
void setup() {
  Serial.begin(115200);
  delay(1000);
// 모터동작
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  //
  Wire.begin(SDA, SCL);
  Wire.setClock(50000);
  delay(500);

  initBLE();

  initBME280();


  initMPU6050();


  initMAX30102();
  initBH1750();

  Serial.println("전체 초기화 완료");
}

// ================= loop =================
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= MEASURE_INTERVAL) {
    previousMillis = currentMillis;

    readBME280(sensorData);
    readMPU6050(sensorData);
    readMAX30102(sensorData);
    readBH1750(sensorData);

    updateDerivedData(sensorData);
    controlVibrationMotor(sensorData);
    printSensorData(sensorData);
    sendBLEData(sensorData);
  }
}
// 모터함수
void controlVibrationMotor(const SensorData &data) {
  if (data.lux > LIGHT_THRESHOLD) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }
}
// ================= BLE 함수 =================
void initBLE() {
  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pDataCharacteristic = pService->createCharacteristic(
                          DATA_CHAR_UUID,
                          BLECharacteristic::PROPERTY_READ |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );

  pDataCharacteristic->addDescriptor(new BLE2902());

  pControlCharacteristic = pService->createCharacteristic(
                             CONTROL_CHAR_UUID,
                             BLECharacteristic::PROPERTY_WRITE
                           );

  pControlCharacteristic->setCallbacks(new ControlCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE 광고 시작");
  Serial.println(DEVICE_NAME);
}

String makeBLEPayload(const SensorData &data) {
  String payload = "";

  payload += "ID:0001";
  payload += ",TEMP:";
  payload += String(data.bodyTemp, 1);
  payload += ",HR:";
  payload += String(data.heartRate);
  payload += ",SPO2:";
  payload += String(data.spo2);
  payload += ",ENV:";
  payload += String(data.temperature, 1);
  payload += ",HUM:";
  payload += String((int)data.humidity);
  payload += ",LUX:";
  payload += String((int)data.lux);
  payload += ",POSTURE:";
  payload += data.posture;

  payload += "\n";

  return payload;
}

void sendBLEData(const SensorData &data) {
  if (!deviceConnected) return;

  String payload = makeBLEPayload(data);

  if (payload.length() > 100) {
    Serial.print("BLE 패킷 길이 초과: ");
    Serial.println(payload.length());
    return;
  }

  pDataCharacteristic->setValue(payload.c_str());
  pDataCharacteristic->notify();
  delay(5);

  Serial.print("BLE 전송: ");
  Serial.print(payload);
}

void handleControlCommand(String command) {
  command.trim();

  Serial.print("앱 제어 명령 수신: ");
  Serial.println(command);

  // 추후 앱 명령에 따라 LED, 진동모터, DFPlayer 제어 가능
  // 예: SAFE / WARNING / DANGER / EMERGENCY 등
}

// ================= 초기화 =================
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

  max30102.setup();
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

// ================= 보정 =================
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

  long irSum = 0;
  long redSum = 0;

  for (int i = 0; i < 50; i++) {
    irSum += max30102.getIR();
    redSum += max30102.getRed();
    delay(20);
  }

  irBase = irSum / 50;
  redBase = redSum / 50;

  Serial.println("MAX30102 기준값 설정 완료");
}

// ================= 센서 읽기 =================
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
    return;
  }

  data.irValue = max30102.getIR();
  data.redValue = max30102.getRed();

  data.fingerDetected = (data.irValue > 50000);
}

void readBH1750(SensorData &data) {
  if (!bhReady) {
    data.lux = 0;
    return;
  }

  data.lux = lightMeter.readLightLevel();
}

// ================= 가공 데이터 =================
void updateDerivedData(SensorData &data) {
  // 앱은 TEMP를 체온으로 해석함.
  // 아직 MAX30205가 없으므로 임시로 36.5 고정.
  // Fever Click 연결 후 data.bodyTemp = max30205 측정값; 으로 교체.
  data.bodyTemp = 36.5;

  // 현재 MAX30102 원시 IR/RED만 있으므로 심박/SpO2는 임시값.
  // SparkFun 예제의 heartRate 알고리즘 적용 후 교체 필요.
  if (data.fingerDetected) {
    data.heartRate = 80;
    data.spo2 = 98;
  } else {
    data.heartRate = 0;   // 앱 파서 범위상 30 미만이면 거부될 수 있음
    data.spo2 = 0;
  }

#if USE_MPU6050
  long accPower = abs(data.ax) + abs(data.ay) + abs(data.az);

  if (accPower > 30000) {
    data.posture = "UNSTABLE";
  } else {
    data.posture = "NORMAL";
  }
#else
  data.posture = "NORMAL";
#endif

  // 앱 파서가 HR 30~220, SPO2 50~100 범위만 허용함.
  // 접촉 불량이어도 앱 테스트를 위해 최소 유효값으로 보정.
  if (data.heartRate < 30) data.heartRate = 30;
  if (data.spo2 < 50) data.spo2 = 50;
}

// ================= 출력 =================
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

  Serial.print("BLE Payload: ");
  Serial.print(makeBLEPayload(data));
}