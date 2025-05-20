#include <Arduino.h>
#include <NimBLEDevice.h>
#define Serial Serial0

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

NimBLECharacteristic* pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("BLE Client Connected");
    }
    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("BLE Client Disconnected");
        NimBLEDevice::startAdvertising();
    }
};

class MyCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        Serial.print("Received: ");
        Serial.println(value.c_str());
        // Echo back
        pCharacteristic->setValue(value);
        pCharacteristic->notify();
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Test");

    NimBLEDevice::init("ESP32_BLE_Test");
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->setValue("Hello BLE Client!");

    pService->start();
    NimBLEDevice::startAdvertising();
    Serial.println("BLE advertising as 'ESP32_BLE_Test'");
}

void loop() {
    // 每秒发送一次心跳
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 1000) {
        Serial.println("Heartbeat...");
        lastHeartbeat = millis();
    }
    // Nothing else needed here for basic BLE echo
} 