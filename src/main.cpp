#include <Arduino.h>
#include <USBCDC.h>
#include "MotorControl.h"
#include "SensorControl.h"
#include "BLEComm.h"
#include "../lib/protocol/lib/OpenBotProtocol.h"

#define Serial Serial0

// 功能开关
#define HAS_BLUETOOTH 1
#define HAS_VOLTAGE_DIVIDER 1
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

// 控制变量
int ctrl_left = 0;
int ctrl_right = 0;
unsigned long heartbeat_time = 0;
unsigned long heartbeat_interval = 1000; // can be updated by protocol heartbeat

// 串口消息处理变量
static char header;
static char msgBuf[60];
static int msgIdx = 0;

// 新增: OpenBot 协议处理器
OpenBotProtocol protocol;

// =========== 协议回调函数 ===========
void onControlCommand(const ControlCommand& cmd) {
    ctrl_left  = cmd.left;
    ctrl_right = cmd.right;
    heartbeat_time = millis();
}

void onHeartbeatCommand(unsigned long interval) {
    if (interval > 0) {
        heartbeat_interval = interval;
    }
    heartbeat_time = millis();
}

// 消息处理回调（BLEComm ➜ OpenBotProtocol）
void handleMessage(char header, const char* body) {
    // 将 BLEComm 提供的 header+body 转为协议字符串并交给解析器
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
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

#if (HAS_BLUETOOTH)
    // 初始化蓝牙
    ble.init();
    ble.setMessageCallback(handleMessage);
#endif
}

void loop() {
    // 每秒串口心跳
    static unsigned long lastSerialHeartbeat = 0;
    if (millis() - lastSerialHeartbeat >= 1000) {
        Serial.println("-- -- -- -- -- -- -- -- Heartbeat -- -- -- -- -- -- -- --");
        lastSerialHeartbeat = millis();
    }

#if (HAS_BLUETOOTH)
    // 更新蓝牙连接状态
    ble.updateConnection();
    // 每3秒蓝牙心跳
    static unsigned long lastBLEHeartbeat = 0;
    if (ble.isConnected() && millis() - lastBLEHeartbeat >= 3000) {
        ble.sendData(">>>>>>>>>>>> BLE Heartbeat <<<<<<<<<<<<<\n");
        lastBLEHeartbeat = millis();
    }
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
        ctrl_left = 0;
        ctrl_right = 0;
    }

    // 更新电机控制
    if (ctrl_left > 0) {
        motors.moveForward(ctrl_left);
    } else if (ctrl_left < 0) {
        motors.moveBackward(-ctrl_left);
    } else if (ctrl_right > 0) {
        motors.turnRight(ctrl_right);
    } else if (ctrl_right < 0) {
        motors.turnLeft(-ctrl_right);
    } else {
        motors.stopMotors();
    }

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