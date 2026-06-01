#include "ble_comm.h"
 #include "actuator.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define DEVICE_NAME "SS_0001"

#define SERVICE_UUID        "089fca17-755f-4578-b8af-ee5e32526b0f"
#define DATA_CHAR_UUID      "0000FFF1-0000-1000-8000-00805F9B34FB"
#define CONTROL_CHAR_UUID   "0000FFF2-0000-1000-8000-00805F9B34FB"

BLEServer *pServer = NULL;
BLECharacteristic *pDataCharacteristic = NULL;
BLECharacteristic *pControlCharacteristic = NULL;

bool deviceConnected = false;

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

void initBLE() {
  BLEDevice::init(DEVICE_NAME);
  BLEDevice::setMTU(128);  // 수정
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
                             BLECharacteristic::PROPERTY_WRITE |
                             BLECharacteristic::PROPERTY_WRITE_NR  // 수정
                           );
  pControlCharacteristic->setCallbacks(new ControlCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // 추가
  pAdvertising->setMinPreferred(0x12);  // 추가
  pAdvertising->start();

  Serial.println("BLE 광고 시작");
  Serial.println(DEVICE_NAME);
}


String makeBLEPayload(const SensorData &data) {
  float axG = data.ax / 16384.0f;
  float ayG = data.ay / 16384.0f;
  float azG = data.az / 16384.0f;

  String payload = "";

  payload += "ID:0001";
  payload += ",";

  payload += "TEMP:";
  payload += String(data.bodyTemp, 1);
  payload += ",";

  payload += "TEMP_VALID:";
  payload += String((data.bodyTemp > 0.0f) ? 1 : 0);
  payload += ",";

  payload += "TEMP_SOURCE:";
  payload += ((data.bodyTemp > 0.0f) ? "M" : "X");
  payload += ",";

  payload += "HR:";
  payload += String(data.heartRate);
  payload += ",";

  payload += "SPO2:";
  payload += String(data.spo2);
  payload += ",";

  payload += "ENV:";
  payload += String(data.temperature, 1);
  payload += ",";

  payload += "HUM:";
  payload += String((int)data.humidity);
  payload += ",";

  payload += "LUX:";
  payload += String((int)data.lux);
  payload += ",";

  payload += "AX:";
  payload += String(axG, 2);
  payload += ",";

  payload += "AY:";
  payload += String(ayG, 2);
  payload += ",";

  payload += "AZ:";
  payload += String(azG, 2);
  payload += ",";

  payload += "POSTURE:";
  payload += data.posture;

  payload += "\n";

  return payload;
}

void sendBLEData(const SensorData &data) {
  if (!deviceConnected) return;

  String payload = makeBLEPayload(data);

  if (payload.length() > 128) {
    Serial.print("BLE 패킷 길이 초과: ");
    Serial.println(payload.length());
    return;
  }

  pDataCharacteristic->setValue(payload.c_str());
  pDataCharacteristic->notify();

  Serial.print("BLE 전송: ");
  Serial.print(payload);
}

// danger.cpp, danger.h 더 이상 위험도 계산 안 함
// ble_comm.cpp
void handleControlCommand(String command) {
  command.trim();
  Serial.print("앱 제어 명령 수신: ");
  Serial.println(command);

  if (command == "RISK:SAFE")           setDangerLevel(0);
  else if (command == "RISK:CAUTION")   setDangerLevel(1);
  else if (command == "RISK:DANGER")    setDangerLevel(2);
  else if (command == "RISK:EMERGENCY") setDangerLevel(3);
}
