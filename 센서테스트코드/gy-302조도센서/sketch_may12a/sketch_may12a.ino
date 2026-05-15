#include <Wire.h>
#include <BH1750.h>

// ================= 핀 설정 =================
#define SDA 21
#define SCL 22

// ================= 센서 객체 =================
BH1750 lightMeter;

// ================= 측정값 =================
float lux = 0.0;

// ================= 타이머 =================
unsigned long previousMillis = 0;
#define MEASURE_INTERVAL 1000

// ================= 함수 선언 =================
void initBH1750();
void readBH1750();
void printSensorData();

// ================= setup =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA, SCL);

  initBH1750();

  Serial.println("조도 센서 초기화 완료");
}

// ================= loop =================
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= MEASURE_INTERVAL) {
    previousMillis = currentMillis;

    readBH1750();
    printSensorData();
  }
}

// ================= 함수 정의 =================

// ---------- 초기화 ----------
void initBH1750() {
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 연결 실패");
    while (1);
  }

  Serial.println("BH1750 시작");
}

// ---------- 센서 읽기 ----------
void readBH1750() {
  lux = lightMeter.readLightLevel();
}

// ---------- 출력 ----------
void printSensorData() {
  Serial.println("================================");

  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  if (lux < 10) {
    Serial.println("상태: 매우 어두움");
  } 
  else if (lux < 100) {
    Serial.println("상태: 어두움");
  } 
  else if (lux < 500) {
    Serial.println("상태: 보통 밝기");
  } 
  else {
    Serial.println("상태: 밝음");
  }
}