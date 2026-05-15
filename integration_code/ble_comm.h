#ifndef BLE_COMM_H
#define BLE_COMM_H

#include "Arduino.h"
#include "sensors.h"

void initBLE();
void sendBLEData(const SensorData &data);
String makeBLEPayload(const SensorData &data);
void handleControlCommand(String command);

#endif