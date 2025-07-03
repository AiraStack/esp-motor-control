#include <Arduino.h>
#include <USBCDC.h>
#include "MotorControl.h"
#include "SensorControl.h"
#include "BLEComm.h"
#include "../lib/protocol/lib/OpenBotProtocol.h"

#define Serial Serial0

// 功能开关
#define HAS_BLUETOOTH 1
#define HAS_VOLTAGE_DIVIDER 0
#define HAS_INDICATORS 1
#define HAS_SONAR 1
#define HAS_SPEED_SENSORS_FRONT 1

// 引脚定义
const uint16_t PIN_PWM_L1 = 21;
const uint16_t PIN_PWM_L2 = 17;
const uint16_t PIN_PWM_R1 = 22;
const uint16_t PIN_PWM_R2 = 23;
const uint16_t PIN_SPEED_LF = 25;
const uint16_t PIN_SPEED_RF = 26;
// const uint16_t PIN_VIN = 39;
// const uint16_t PIN_TRIGGER = 25;
// const uint16_t PIN_ECHO = 26;
// const uint16_t PIN_LED_LI = 22;
// const uint16_t PIN_LED_RI = 16;

// 创建对象
MotorControl motors(PIN_SPEED_LF, PIN_PWM_L1, PIN_PWM_L2, PIN_SPEED_RF, PIN_PWM_R1, PIN_PWM_R2);
// SensorControl sensors(PIN_TRIGGER, PIN_ECHO, PIN_VIN, PIN_LED_LI, PIN_LED_RI);
#if (HAS_BLUETOOTH)
BLEComm ble("OpenBot: DIY_ESP32");
#endif

unsigned long heartbeat_time = 0;
unsigned long heartbeat_interval = 1000; // can be updated by protocol heartbeat

// 控制变量
struct MotorState {
    int left = 0;
    int right = 0;
    unsigned long lastUpdate = 0;
    bool isActive() const {
        return (millis() - lastUpdate) < heartbeat_interval;
    }
    void stop() {
        left = right = 0;
    }
} motorState;

// 串口消息处理变量
static char header;
static char msgBuf[60];
static int msgIdx = 0;

// 新增: OpenBot 协议处理器
OpenBotProtocol protocol;

// 电机控制函数
void updateMotorControl() {
    // 心跳超时检查
    if (!motorState.isActive()) {
        motorState.stop();
    }
    
    // 差分驱动控制
    int left = motorState.left;
    int right = motorState.right;
    
    if (left == 0 && right == 0) {
        motors.stopMotors();
    } else if (left == right) {
        // 直线运动
        if (left > 0) {
            motors.moveForward(abs(left));
        } else {
            motors.moveBackward(abs(left));
        }
    } else {
        // 转向运动 - 简化版，可根据需要改进
        if (abs(left) > abs(right)) {
            motors.turnLeft(abs(left - right));
        } else {
            motors.turnRight(abs(right - left));
        }
    }
}

// =========== 协议回调函数 ===========
void onControlCommand(const ControlCommand& cmd) {
    Serial.printf("Motor Control via BLE - Left: %d, Right: %d\n", cmd.left, cmd.right);
    motorState.left = cmd.left;
    motorState.right = cmd.right;
    motorState.lastUpdate = millis();
}

void onHeartbeatCommand(unsigned long interval) {
    if (interval > 0) {
        heartbeat_interval = interval;
    }
    motorState.lastUpdate = millis();
}

// 发送数据回调 - 同时发送到Serial0和BLE
void onSendData(const char* data) {
    Serial.printf("DEBUG: Sending data: '%s'\n", data);
    
    // 发送到串口
    Serial.print(data);
    
#if (HAS_BLUETOOTH)
    // 发送到蓝牙（如果连接）
    // 暂时禁用BLE自动响应，避免spam
    /*if (ble.isConnected()) {
        ble.sendData(data);
    }*/
#endif
}

// 消息处理回调（BLEComm ➜ OpenBotProtocol）
void handleMessage(char header, const char* body) {
    const char* realBody = body;
    if (header == body[0]) {
        realBody = body + 1;
    }
    // 将 BLEComm 提供的 header+body 转为协议字符串并交给解析器
    String message = String(header) + String(body);
    Serial.printf(">>>>>>>>>> handleMessage: DEBUG: Received message: '%s', header: '%c', body: '%s'\n", message.c_str(), header, realBody);
    protocol.processSerialMessage(message.c_str());
}

// 测试协议解析
void testProtocolParsing() {
    Serial.println("\n=== Protocol Parsing Test ===");
    
    // 测试控制命令
    Serial.println("Testing: c100,-50");
    protocol.processSerialMessage("c100,-50");
    
    Serial.println("Testing: c0,0");
    protocol.processSerialMessage("c0,0");
    
    Serial.println("Testing: l255,128");
    protocol.processSerialMessage("l255,128");
    
    Serial.println("=== Test Complete ===\n");
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    Serial.println('r');

    // 初始化电机控制
    motors.init();

    // 初始化传感器
    // sensors.init();

    // 初始化协议处理器并注册回调
    protocol.init();
    protocol.setControlCallback(onControlCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    protocol.setSendCallback(onSendData);

#if (HAS_BLUETOOTH)
    // 初始化蓝牙
    ble.init();
    ble.setMessageCallback(handleMessage);
#endif

    // 延迟3秒后测试协议解析
    delay(3000);
    testProtocolParsing();
}

void loop() {
    // 每秒串口心跳
    static unsigned long lastSerialHeartbeat = 0;
    if (millis() - lastSerialHeartbeat >= 5000) {
        Serial.println("-- -- -- -- -- -- -- -- Heartbeat -- -- -- -- -- -- -- --");
        lastSerialHeartbeat = millis();
    }

#if (HAS_BLUETOOTH)
    // 更新蓝牙连接状态
    ble.updateConnection();
#endif

    // 检查串口消息
    if (Serial.available() > 0) {
        char inChar = Serial.read();
        if (inChar != '\n') {
            if (msgIdx == 0) {
                header = inChar;
                msgIdx++;
            } else if (msgIdx < 60) {
                msgBuf[msgIdx - 1] = inChar;
                msgIdx++;
            }
        } else {
            msgBuf[msgIdx - 1] = '\0';
            // 将串口收到的消息也交给协议解析器
            String serialMsg = String(header) + String(msgBuf);
            protocol.processSerialMessage(serialMsg.c_str());
            msgIdx = 0;
        }
    }

    // 检查距离并停止
#if (HAS_SONAR)
    // if (sensors.getDistance() <= 10 && ctrl_left > 0 && ctrl_right > 0) {
    //     ctrl_left = 0;
    //     ctrl_right = 0;
    // }
#endif

    // 检查心跳超时
    if ((millis() - heartbeat_time) >= heartbeat_interval) {
        motorState.stop();
    }

    // 更新电机控制
    updateMotorControl();

#if (HAS_VOLTAGE_DIVIDER)
    // 更新电压监测
    // sensors.updateVoltage();
    // if (sensors.isLowBattery()) {
    //     motors.stopMotors();
    // }
#endif

#if (HAS_SONAR)
    // 更新超声波测距
    // sensors.startPing(); 
    // if (sensors.checkEcho()) {
    //     sensors.updateDistanceEstimate();
    //     char buf[10];
    //     sprintf(buf, "d%d\n", sensors.getDistance());
    //     Serial.print(buf);
// #if (HAS_BLUETOOTH)
//         ble.sendData(buf);
// #endif
//     }
#endif

#if (HAS_VOLTAGE_DIVIDER)
    // 发送电压读数
    static unsigned long voltage_time = 0;
    if ((millis() - voltage_time) >= 1000) {
        char buf[10];
        // sprintf(buf, "v%.1f\n", sensors.getVoltage());
        Serial.print(buf);
#if (HAS_BLUETOOTH)
        ble.sendData(buf);
#endif
        voltage_time = millis();
    }
#endif
} 