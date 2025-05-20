#ifndef BLE_COMM_H
#define BLE_COMM_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define Serial Serial0

class BLEComm {
public:
    // 构造函数
    BLEComm(const char* deviceName);

    // 初始化函数
    void init();

    // 连接状态
    bool isConnected();
    void updateConnection();

    // 发送数据
    void sendData(const char* data);
    void sendData(const uint8_t* data, size_t length);

    // 回调函数类型定义
    typedef void (*MessageCallback)(char header, const char* body);

    // 设置消息回调
    void setMessageCallback(MessageCallback callback);

private:
    const char* _deviceName;
    BLEServer* _bleServer;
    BLECharacteristic* _pTxCharacteristic;
    BLECharacteristic* _pRxCharacteristic;
    bool _deviceConnected;
    bool _oldDeviceConnected;
    MessageCallback _messageCallback;

    // 消息处理相关变量
    enum msgParts {
        HEADER,
        BODY
    };
    msgParts _msgPart;
    char _header;
    char _endChar;
    const char _MAX_MSG_SZ;
    char _msgBuf[60];
    int _msgIdx;

    // 内部函数
    void processHeader(char inChar);
    void processBody(char inChar);
    void parseMessage();

    // 蓝牙回调类
    class MyServerCallbacks : public BLEServerCallbacks {
        BLEComm* _parent;
    public:
        MyServerCallbacks(BLEComm* parent) : _parent(parent) {}
        void onConnect(BLEServer* bleServer, esp_ble_gatts_cb_param_t* param);
        void onDisconnect(BLEServer* bleServer);
    };

    class MyCallbacks : public BLECharacteristicCallbacks {
        BLEComm* _parent;
    public:
        MyCallbacks(BLEComm* parent) : _parent(parent) {}
        void onWrite(BLECharacteristic* pCharacteristic);
    };
};

#endif // BLE_COMM_H 