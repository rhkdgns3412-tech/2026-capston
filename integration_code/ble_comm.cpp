#include "ble_comm.h"
#include "danger.h"

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
                             BLECharacteristic::PROPERTY_WRITE
                           );

  pControlCharacteristic->setCallbacks(new ControlCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE 광고 시작");
  Serial.println(DEVICE_NAME);
}

String makeBLEPayload(const SensorData &data) {
  // Produce a simple CSV line (no headers):
  // ID,TEMP,TEMP_VALID,TEMP_SOURCE,HR,SPO2,ENV,HUM,LUX,AX,AY,AZ,POSTURE\n
  String payload = "";

  payload += "0001"; // device ID
  payload += ",";
  payload += String(data.bodyTemp, 1);
  payload += ",";
  payload += String((data.bodyTemp > 0.0f) ? 1 : 0);
  payload += ",";
  payload += ((data.bodyTemp > 0.0f) ? "MAX30205" : "NONE");
  payload += ",";
  payload += String(data.heartRate);
  payload += ",";
  payload += String(data.spo2);
  payload += ",";
  payload += String(data.temperature, 1);
  payload += ",";
  payload += String((int)data.humidity);
  payload += ",";
  payload += String((int)data.lux);
  payload += ",";
  payload += String(data.ax);
  payload += ",";
  payload += String(data.ay);
  payload += ",";
  payload += String(data.az);
  payload += ",";
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

void handleControlCommand(String command) {
  command.trim();

  Serial.print("앱 제어 명령 수신: ");
  Serial.println(command);
}