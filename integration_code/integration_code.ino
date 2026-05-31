#include "sensors.h"
#include "ble_comm.h"
#include "actuator.h"
#include "danger.h"

SensorData sensorData;

unsigned long previousSensorMillis = 0;
unsigned long previousReportMillis = 0;

// 데모용 센서 샘플링 주기 및 출력 주기: 500ms
#define SENSOR_INTERVAL 500
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