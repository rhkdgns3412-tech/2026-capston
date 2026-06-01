#include "sensors.h"
#include "ble_comm.h"
#include "actuator.h"


SensorData sensorData;

unsigned long previousMillis = 0;

// 데모용 센서 샘플링 주기 및 출력 주기: 1000ms
#define LOOP_INTERVAL 1000
void setup() {
  Serial.begin(115200);
  delay(1000);
  sensorData.temperature = 25.0;
  sensorData.humidity = 50;
  sensorData.heartRate = 30;
  sensorData.spo2 = 50;
  initActuators();
  initSensors();
  initBLE();

  Serial.println("전체 초기화 완료");
}

void loop(){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= LOOP_INTERVAL) {
    previousMillis = currentMillis;

    readSensors(sensorData);     
    updateDerivedData(sensorData);
    controlActuators(currentDangerLevel);
    printSensorData(sensorData);
    sendBLEData(sensorData);   
  }
}
