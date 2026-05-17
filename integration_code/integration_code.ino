#include "sensors.h"
#include "ble_comm.h"
#include "actuator.h"

SensorData sensorData;

unsigned long previousSensorMillis = 0;
unsigned long previousReportMillis = 0;

// 센서 샘플링 주기: MAX30102 알고리즘이 동작할 수 있도록 25Hz 근처로 유지
#define SENSOR_INTERVAL 40
// 출력/BLE 전송 주기
#define REPORT_INTERVAL 500

void setup() {
  Serial.begin(115200);
  delay(1000);

  initActuators();
  initSensors();
  initBLE();

  Serial.println("전체 초기화 완료");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousSensorMillis >= SENSOR_INTERVAL) {
    previousSensorMillis = currentMillis;

    readSensors(sensorData);
    updateDerivedData(sensorData);

    controlActuators(sensorData);
  }

  if (currentMillis - previousReportMillis >= REPORT_INTERVAL) {
    previousReportMillis = currentMillis;

    printSensorData(sensorData);
    sendBLEData(sensorData);
  }
}