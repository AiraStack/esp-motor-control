#include "OpenBotProtocol.h"

OpenBotProtocol::OpenBotProtocol() 
    : msgPart(HEADER)
    , header('\0')
    , endChar('\n')
    , msgIndex(0)
    , lastHeartbeat(0)
    , controlCallback(nullptr)
    , lightCallback(nullptr)
    , indicatorCallback(nullptr)
    , heartbeatCallback(nullptr)
    , notificationCallback(nullptr)
    , jsonControlCallback(nullptr)
    , jsonCommandCallback(nullptr)
    , sendCallback(nullptr)
{
    memset(msgBuffer, 0, MAX_MSG_SIZE);
}

void OpenBotProtocol::init() {
    resetParser();
    lastHeartbeat = millis();
}

void OpenBotProtocol::processSerialData(char data) {
    if (data != endChar) {
        switch (msgPart) {
            case HEADER:
                processHeader(data);
                break;
            case BODY:
                processBody(data);
                break;
        }
    } else {
        msgBuffer[msgIndex] = '\0';  // 结束字符串
        parseSerialMessage();
    }
}

void OpenBotProtocol::processSerialMessage(const char* message) {
    if (strlen(message) < 2) return;
    
    header = message[0];
    strcpy(msgBuffer, message + 1);
    parseSerialMessage();
}

bool OpenBotProtocol::parseJsonMessage(const char* jsonStr) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        return false;
    }
    
    // 检查是否是驱动命令
    if (doc.containsKey("driveCmd")) {
        return parseJsonDriveCommand(doc);
    }
    // 检查是否是普通命令
    else if (doc.containsKey("command")) {
        return parseJsonCommand(doc);
    }
    
    return false;
}

void OpenBotProtocol::sendControlResponse(int left, int right) {
    if (sendCallback) {
        String response = "c" + String(left) + "," + String(right) + "\n";
        sendCallback(response.c_str());
    }
}

void OpenBotProtocol::sendSensorData(const char* sensorType, float value) {
    if (sendCallback) {
        String response = String(sensorType) + String(value, 2) + "\n";
        sendCallback(response.c_str());
    }
}

void OpenBotProtocol::sendFeatureResponse(const char* features) {
    if (sendCallback) {
        String response = "f" + String(features) + "\n";
        sendCallback(response.c_str());
    }
}

void OpenBotProtocol::sendVoltageInfo(float vmin, float vlow, float vmax) {
    if (sendCallback) {
        String response = "vmin:" + String(vmin, 2) + "\nvlow:" + String(vlow, 2) + "\nvmax:" + String(vmax, 2) + "\n";
        sendCallback(response.c_str());
    }
}

// 回调函数设置
void OpenBotProtocol::setControlCallback(ControlCallback callback) {
    controlCallback = callback;
}

void OpenBotProtocol::setLightCallback(LightCallback callback) {
    lightCallback = callback;
}

void OpenBotProtocol::setIndicatorCallback(IndicatorCallback callback) {
    indicatorCallback = callback;
}

void OpenBotProtocol::setHeartbeatCallback(HeartbeatCallback callback) {
    heartbeatCallback = callback;
}

void OpenBotProtocol::setNotificationCallback(NotificationCallback callback) {
    notificationCallback = callback;
}

void OpenBotProtocol::setJsonControlCallback(JsonControlCallback callback) {
    jsonControlCallback = callback;
}

void OpenBotProtocol::setJsonCommandCallback(JsonCommandCallback callback) {
    jsonCommandCallback = callback;
}

void OpenBotProtocol::setSendCallback(SendCallback callback) {
    sendCallback = callback;
}

bool OpenBotProtocol::isConnected() const {
    return (millis() - lastHeartbeat) < 1000;  // 1秒内有心跳认为连接正常
}

unsigned long OpenBotProtocol::getLastHeartbeat() const {
    return lastHeartbeat;
}

// 静态工具函数
String OpenBotProtocol::createJsonDriveCommand(float left, float right) {
    DynamicJsonDocument doc(256);
    JsonObject driveCmd = doc.createNestedObject("driveCmd");
    driveCmd["l"] = left;
    driveCmd["r"] = right;
    
    String result;
    serializeJson(doc, result);
    return result;
}

String OpenBotProtocol::createJsonCommand(const char* command) {
    DynamicJsonDocument doc(256);
    doc["command"] = command;
    
    String result;
    serializeJson(doc, result);
    return result;
}

void OpenBotProtocol::floatToSerialRange(float value, int& result) {
    // 将 -1.0 到 1.0 的浮点值转换为 -255 到 255 的整数值
    result = (int)(value * 255.0f);
    result = constrain(result, -255, 255);
}

float OpenBotProtocol::serialToFloatRange(int value) {
    // 将 -255 到 255 的整数值转换为 -1.0 到 1.0 的浮点值
    return constrain(value, -255, 255) / 255.0f;
}

// 私有函数实现
void OpenBotProtocol::parseSerialMessage() {
    switch (header) {
        case 'c':
            processControlMessage();
            break;
        case 'l':
            processLightMessage();
            break;
        case 'i':
            processIndicatorMessage();
            break;
        case 'h':
            processHeartbeatMessage();
            break;
        case 'f':
            processFeatureMessage();
            break;
        case 'v':
            processVoltageMessage();
            break;
        case 's':
            processSonarMessage();
            break;
        case 'b':
            processBumperMessage();
            break;
        case 'w':
            processWheelMessage();
            break;
        case 'n':
            processNotificationMessage();
            break;
    }
    resetParser();
}

void OpenBotProtocol::processControlMessage() {
    char* tmp;
    tmp = strtok(msgBuffer, ",:");
    if (tmp == nullptr) return;
    
    ControlCommand cmd;
    cmd.left = atoi(tmp);
    
    tmp = strtok(nullptr, ",:");
    if (tmp == nullptr) return;
    cmd.right = atoi(tmp);
    
    if (controlCallback) {
        controlCallback(cmd);
    }
}

void OpenBotProtocol::processLightMessage() {
    char* tmp;
    tmp = strtok(msgBuffer, ",:");
    if (tmp == nullptr) return;
    
    LightCommand cmd;
    cmd.front = atoi(tmp);
    
    tmp = strtok(nullptr, ",:");
    if (tmp == nullptr) return;
    cmd.back = atoi(tmp);
    
    if (lightCallback) {
        lightCallback(cmd);
    }
}

void OpenBotProtocol::processIndicatorMessage() {
    char* tmp;
    tmp = strtok(msgBuffer, ",:");
    if (tmp == nullptr) return;
    
    IndicatorCommand cmd;
    cmd.left = atoi(tmp);
    
    tmp = strtok(nullptr, ",:");
    if (tmp == nullptr) return;
    cmd.right = atoi(tmp);
    
    if (indicatorCallback) {
        indicatorCallback(cmd);
    }
}

void OpenBotProtocol::processHeartbeatMessage() {
    unsigned long interval = atol(msgBuffer);
    lastHeartbeat = millis();
    
    if (heartbeatCallback) {
        heartbeatCallback(interval);
    }
}

void OpenBotProtocol::processFeatureMessage() {
    // 功能查询，发送支持的功能列表
    // 这里应该根据实际硬件配置来发送
    sendFeatureResponse("ESP32:v:i:s:wf:lf:lb:ls:");
}

void OpenBotProtocol::processVoltageMessage() {
    unsigned long interval = atol(msgBuffer);
    // 发送电压范围信息
    sendVoltageInfo(2.5, 9.0, 12.6);
}

void OpenBotProtocol::processSonarMessage() {
    // 处理声纳传感器配置
    unsigned long interval = atol(msgBuffer);
    // 可以在这里设置传感器读取间隔
}

void OpenBotProtocol::processBumperMessage() {
    // 处理碰撞传感器配置
    unsigned long interval = atol(msgBuffer);
    // 可以在这里设置传感器读取间隔
}

void OpenBotProtocol::processWheelMessage() {
    // 处理轮速传感器配置
    unsigned long interval = atol(msgBuffer);
    // 可以在这里设置传感器读取间隔
}

void OpenBotProtocol::processNotificationMessage() {
    char* tmp;
    tmp = strtok(msgBuffer, ",:");
    if (tmp == nullptr) return;
    
    NotificationCommand cmd;
    cmd.led = tmp[0];
    
    tmp = strtok(nullptr, ",:");
    if (tmp == nullptr) return;
    cmd.state = atoi(tmp);
    
    if (notificationCallback) {
        notificationCallback(cmd);
    }
}

void OpenBotProtocol::processHeader(char inChar) {
    header = inChar;
    msgPart = BODY;
}

void OpenBotProtocol::processBody(char inChar) {
    if (msgIndex < MAX_MSG_SIZE - 1) {
        msgBuffer[msgIndex] = inChar;
        msgIndex++;
    }
}

void OpenBotProtocol::resetParser() {
    msgIndex = 0;
    msgPart = HEADER;
    header = '\0';
    memset(msgBuffer, 0, MAX_MSG_SIZE);
}

bool OpenBotProtocol::parseJsonDriveCommand(JsonDocument& doc) {
    JsonObject driveCmd = doc["driveCmd"];
    if (driveCmd.isNull()) return false;
    
    JsonDriveCommand cmd;
    cmd.left = driveCmd["l"];
    cmd.right = driveCmd["r"];
    
    if (jsonControlCallback) {
        jsonControlCallback(cmd);
    }
    return true;
}

bool OpenBotProtocol::parseJsonCommand(JsonDocument& doc) {
    const char* command = doc["command"];
    if (command == nullptr) return false;
    
    if (jsonCommandCallback) {
        jsonCommandCallback(command);
    }
    return true;
} 