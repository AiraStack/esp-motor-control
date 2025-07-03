#ifndef OPENBOT_PROTOCOL_H
#define OPENBOT_PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>

// 命令类型枚举
enum class CommandType {
    UNKNOWN = 0,
    CONTROL,      // 电机控制
    LIGHT,        // LED控制
    INDICATOR,    // 指示灯控制
    HEARTBEAT,    // 心跳
    FEATURE,      // 功能查询
    VOLTAGE,      // 电压查询
    SONAR,        // 声纳传感器
    BUMPER,       // 碰撞传感器
    WHEEL,        // 轮速传感器
    NOTIFICATION  // 状态LED通知
};

// 控制命令结构
struct ControlCommand {
    int left;   // 左轮控制值 (-255 到 255)
    int right;  // 右轮控制值 (-255 到 255)
};

// 灯光控制命令结构
struct LightCommand {
    int front;  // 前灯亮度 (0-255)
    int back;   // 后灯亮度 (0-255)
};

// 指示灯控制命令结构
struct IndicatorCommand {
    int left;   // 左指示灯状态 (0/1)
    int right;  // 右指示灯状态 (0/1)
};

// 状态LED通知命令结构
struct NotificationCommand {
    char led;   // LED颜色 ('y', 'g', 'b')
    int state;  // LED状态 (0/1)
};

// JSON驱动命令结构（用于网络协议）
struct JsonDriveCommand {
    float left;   // 左轮速度 (-1.0 到 1.0)
    float right;  // 右轮速度 (-1.0 到 1.0)
};

// 回调函数类型定义
typedef void (*ControlCallback)(const ControlCommand& cmd);
typedef void (*LightCallback)(const LightCommand& cmd);
typedef void (*IndicatorCallback)(const IndicatorCommand& cmd);
typedef void (*HeartbeatCallback)(unsigned long interval);
typedef void (*NotificationCallback)(const NotificationCommand& cmd);
typedef void (*JsonControlCallback)(const JsonDriveCommand& cmd);
typedef void (*JsonCommandCallback)(const char* command);

// 发送数据回调函数类型 (用户决定如何发送：Serial/BLE/WiFi等)
typedef void (*SendCallback)(const char* data);

class OpenBotProtocol {
public:
    // 构造函数
    OpenBotProtocol();
    
    // 初始化
    void init();
    
    // 串口协议处理
    void processSerialData(char data);
    void processSerialMessage(const char* message);
    
    // JSON协议处理（用于网络通信）
    bool parseJsonMessage(const char* jsonStr);
    
    // 命令发送函数
    void sendControlResponse(int left, int right);
    void sendSensorData(const char* sensorType, float value);
    void sendFeatureResponse(const char* features);
    void sendVoltageInfo(float vmin, float vlow, float vmax);
    
    // 回调函数设置
    void setControlCallback(ControlCallback callback);
    void setLightCallback(LightCallback callback);
    void setIndicatorCallback(IndicatorCallback callback);
    void setHeartbeatCallback(HeartbeatCallback callback);
    void setNotificationCallback(NotificationCallback callback);
    void setJsonControlCallback(JsonControlCallback callback);
    void setJsonCommandCallback(JsonCommandCallback callback);
    void setSendCallback(SendCallback callback);
    
    // 协议状态查询
    bool isConnected() const;
    unsigned long getLastHeartbeat() const;
    
    // 工具函数
    static String createJsonDriveCommand(float left, float right);
    static String createJsonCommand(const char* command);
    static void floatToSerialRange(float value, int& result);
    static float serialToFloatRange(int value);

private:
    // 串口协议状态
    enum MessagePart {
        HEADER,
        BODY
    };
    
    MessagePart msgPart;
    char header;
    char endChar;
    static const int MAX_MSG_SIZE = 64;
    char msgBuffer[MAX_MSG_SIZE];
    int msgIndex;
    unsigned long lastHeartbeat;
    
    // 回调函数指针
    ControlCallback controlCallback;
    LightCallback lightCallback;
    IndicatorCallback indicatorCallback;
    HeartbeatCallback heartbeatCallback;
    NotificationCallback notificationCallback;
    JsonControlCallback jsonControlCallback;
    JsonCommandCallback jsonCommandCallback;
    SendCallback sendCallback;
    
    // 内部处理函数
    void parseSerialMessage();
    void processControlMessage();
    void processLightMessage();
    void processIndicatorMessage();
    void processHeartbeatMessage();
    void processFeatureMessage();
    void processVoltageMessage();
    void processSonarMessage();
    void processBumperMessage();
    void processWheelMessage();
    void processNotificationMessage();
    
    void processHeader(char inChar);
    void processBody(char inChar);
    void resetParser();
    
    // JSON解析辅助函数
    bool parseJsonDriveCommand(JsonDocument& doc);
    bool parseJsonCommand(JsonDocument& doc);
};

#endif // OPENBOT_PROTOCOL_H 