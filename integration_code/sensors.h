#ifndef SENSORS_H
#define SENSORS_H

#include "Arduino.h"

#define SDA 21
#define SCL 22
#define BUFFER_SIZE 100  // MAX30102 버퍼 크기
// MAX30102 샘플링 간격은 40ms로 사용한다.
// HR 계산식에서 쓰는 Ts도 이 값과 동일하게 맞춰야 한다.
// HR(bpm) = 60000 / (평균 피크 간격(샘플) * 40ms)



struct SensorData {
  float temperature;
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

void initSensors();
void readSensors(SensorData &data);
void updateDerivedData(SensorData &data);
void printSensorData(const SensorData &data);

#endif

