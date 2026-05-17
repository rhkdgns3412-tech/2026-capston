#ifndef SENSORS_H
#define SENSORS_H

#include "Arduino.h"

#define SDA 21
#define SCL 22



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

