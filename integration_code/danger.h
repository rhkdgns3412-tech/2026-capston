#ifndef DANGER_H
#define DANGER_H

#include "Arduino.h"
#include "sensors.h"

#define DEFAULT_BASELINE_TEMP 37.0
#define DEFAULT_BASELINE_HR 80

int calculateDangerLevel(const SensorData &data, double baselineTemp, int baselineHR);

#endif