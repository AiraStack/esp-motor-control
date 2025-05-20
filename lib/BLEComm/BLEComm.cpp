#include "BLEComm.h"

BLEComm::BLEComm(const char* deviceName)
    : _deviceName(deviceName),
      _bleServer(nullptr),
      _pTxCharacteristic(nullptr),
      _pRxCharacteristic(nullptr),
      _deviceConnected(false),
      _oldDeviceConnected(false),
      _messageCallback(nullptr),
      _msgPart(HEADER),
      _endChar('\n'),
      _MAX_MSG_SZ(60),
      _msgIdx(0)
{
}

void BLEComm::init() {
    BLEDevice::init("OpenBot: DIY_ESP32");
    _bleServer = BLEDevice::createServer();
    _bleServer->setCallbacks(new MyServerCallbacks(this));

    BLEService* pService = _bleServer->createService(BLEUUID("61653dc3-4021-4d1e-ba83-8b4eec61d613"));

    _pTxCharacteristic = pService->createCharacteristic(
        BLEUUID("9bf1103b-834c-47cf-b149-c9e4bcf778a7"),
        NIMBLE_PROPERTY::NOTIFY
    );
    // _pTxCharacteristic->addDescriptor(new NimBLE2902());

    _pRxCharacteristic = pService->createCharacteristic(
        BLEUUID("06386c14-86ea-4d71-811c-48f97c58f8c9"),
        NIMBLE_PROPERTY::WRITE_NR
    );
    _pRxCharacteristic->setCallbacks(new MyCallbacks(this));
    // _pRxCharacteristic->addDescriptor(new NimBLE2902());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID("61653dc3-4021-4d1e-ba83-8b4eec61d613"));
    _bleServer->getAdvertising()->start();
}

bool BLEComm::isConnected() {
    return _deviceConnected;
}

void BLEComm::updateConnection() {
    if (!_deviceConnected && _oldDeviceConnected) {
        delay(500);
        _bleServer->startAdvertising();
        _oldDeviceConnected = _deviceConnected;
    }
    if (_deviceConnected && !_oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
    }
}

void BLEComm::sendData(const char* data) {
    if (_deviceConnected) {
        _pTxCharacteristic->setValue((uint8_t*)data, strlen(data));
        _pTxCharacteristic->notify();
    }
}

void BLEComm::sendData(const uint8_t* data, size_t length) {
    if (_deviceConnected) {
        _pTxCharacteristic->setValue(std::string((const char*)data, length));
        _pTxCharacteristic->notify();
    }
}

void BLEComm::setMessageCallback(MessageCallback callback) {
    _messageCallback = callback;
}

void BLEComm::processHeader(char inChar) {
    _header = inChar;
    _msgPart = BODY;
    _msgIdx = 0;
}

void BLEComm::processBody(char inChar) {
    if (_msgIdx < _MAX_MSG_SZ - 1) {
        _msgBuf[_msgIdx++] = inChar;
    }
}

void BLEComm::parseMessage() {
    if (_messageCallback) {
        _msgBuf[_msgIdx] = '\0';
        _messageCallback(_header, _msgBuf);
    }
    _msgPart = HEADER;
}

void BLEComm::MyServerCallbacks::onConnect(BLEServer* bleServer) {
    _parent->_deviceConnected = true;
    Serial.println("BT Connected");
}

void BLEComm::MyServerCallbacks::onDisconnect(BLEServer* bleServer) {
    _parent->_deviceConnected = false;
    Serial.println("BT Disconnected");
}

void BLEComm::MyCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        for (int i = 0; i < value.length(); i++) {
            char inChar = value[i];
            if (inChar != _parent->_endChar) {
                switch (_parent->_msgPart) {
                    case HEADER:
                        _parent->processHeader(inChar);
                        break;
                    case BODY:
                        _parent->processBody(inChar);
                        break;
                }
            } else {
                _parent->parseMessage();
            }
        }
    }
} 