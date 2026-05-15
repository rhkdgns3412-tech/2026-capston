#include "sensors.h"
#include "ble_comm.h"
#include "actuator.h"

SensorData sensorData;

unsigned long previousMillis = 0;
//측정주기
#define MEASURE_INTERVAL 500

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

  if (currentMillis - previousMillis >= MEASURE_INTERVAL) {
    previousMillis = currentMillis;

    readSensors(sensorData);
    updateDerivedData(sensorData);

    controlActuators(sensorData);
    printSensorData(sensorData);
    sendBLEData(sensorData);
  }
}