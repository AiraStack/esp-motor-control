#include <OpenBotProtocol.h>
#include <BLEComm.h>
#include <MotorControl.h>

// ============= 硬件配置 =============
#define MOTOR_LEFT_PWM   5
#define MOTOR_LEFT_DIR1  18
#define MOTOR_LEFT_DIR2  19
#define MOTOR_RIGHT_PWM  16
#define MOTOR_RIGHT_DIR1 17
#define MOTOR_RIGHT_DIR2 4

#define FRONT_LED_PIN    21
#define BACK_LED_PIN     22
#define STATUS_LED_Y     23
#define STATUS_LED_G     25
#define STATUS_LED_B     26

// ============= 对象实例 =============
OpenBotProtocol protocol;
BLEComm bleComm("OpenBot-ESP32");
MotorControl motors(MOTOR_LEFT_PWM, MOTOR_LEFT_DIR1, MOTOR_LEFT_DIR2,
                   MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIR1, MOTOR_RIGHT_DIR2);

// ============= 蓝牙消息处理 =============
void onBleMessage(char header, const char* body) {
    Serial.printf("BLE Received: %c%s\n", header, body);
    
    // 将BLE消息转发给协议处理器
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
}

// ============= 协议回调函数 =============
void onControlCommand(const ControlCommand& cmd) {
    Serial.printf("Motor Control via BLE - Left: %d, Right: %d\n", cmd.left, cmd.right);
    
    // 控制电机
    if (cmd.left == 0 && cmd.right == 0) {
        motors.stopMotors();
    } else if (cmd.left == cmd.right) {
        if (cmd.left > 0) {
            motors.moveForward(abs(cmd.left));
        } else {
            motors.moveBackward(abs(cmd.left));
        }
    } else if (cmd.left > cmd.right) {
        motors.turnLeft(abs(cmd.left - cmd.right));
    } else {
        motors.turnRight(abs(cmd.right - cmd.left));
    }
    
    // 通过BLE发送状态反馈
    String response = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

void onLightCommand(const LightCommand& cmd) {
    Serial.printf("Light Control via BLE - Front: %d, Back: %d\n", cmd.front, cmd.back);
    
    analogWrite(FRONT_LED_PIN, cmd.front);
    analogWrite(BACK_LED_PIN, cmd.back);
    
    // BLE响应
    String response = "l" + String(cmd.front) + "," + String(cmd.back);
    bleComm.sendData(response.c_str());
}

void onIndicatorCommand(const IndicatorCommand& cmd) {
    Serial.printf("Indicator via BLE - Left: %d, Right: %d\n", cmd.left, cmd.right);
    
    // 处理指示灯逻辑...
    
    // BLE响应
    String response = "i" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(response.c_str());
}

void onHeartbeatCommand(unsigned long interval) {
    Serial.printf("Heartbeat via BLE - Interval: %lu ms\n", interval);
    
    // 发送系统状态
    sendSystemStatus();
}

void onNotificationCommand(const NotificationCommand& cmd) {
    Serial.printf("Notification via BLE - LED: %c, State: %d\n", cmd.led, cmd.state);
    
    // 控制状态LED
    switch (cmd.led) {
        case 'y':
            digitalWrite(STATUS_LED_Y, cmd.state);
            break;
        case 'g':
            digitalWrite(STATUS_LED_G, cmd.state);
            break;
        case 'b':
            digitalWrite(STATUS_LED_B, cmd.state);
            break;
    }
}

// ============= 辅助函数 =============
void sendSystemStatus() {
    // 发送电池电压（示例值）
    float voltage = 11.5;
    String voltageMsg = "v" + String(voltage, 2);
    bleComm.sendData(voltageMsg.c_str());
    
    // 发送功能支持信息
    bleComm.sendData("fESP32:v:i:s:wf:lf:lb:ls:");
    
    // 发送电压范围信息
    bleComm.sendData("vmin:2.50");
    bleComm.sendData("vlow:9.00");
    bleComm.sendData("vmax:12.60");
}

void setup() {
    Serial.begin(115200);
    
    // 初始化引脚
    pinMode(FRONT_LED_PIN, OUTPUT);
    pinMode(BACK_LED_PIN, OUTPUT);
    pinMode(STATUS_LED_Y, OUTPUT);
    pinMode(STATUS_LED_G, OUTPUT);
    pinMode(STATUS_LED_B, OUTPUT);
    
    // 初始化模块
    motors.init();
    protocol.init();
    
    // 设置协议回调函数
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    protocol.setNotificationCallback(onNotificationCommand);
    
    // 初始化BLE并设置消息回调
    bleComm.init();
    bleComm.setMessageCallback(onBleMessage);  // 关键：将BLE消息转发给协议处理器
    
    // 状态指示
    digitalWrite(STATUS_LED_Y, HIGH);
    Serial.println("System initialized. Waiting for BLE connection...");
    
    // 等待BLE连接
    while (!bleComm.isConnected()) {
        bleComm.updateConnection();
        delay(100);
        digitalWrite(STATUS_LED_Y, !digitalRead(STATUS_LED_Y)); // 闪烁表示等待连接
    }
    
    digitalWrite(STATUS_LED_Y, LOW);
    digitalWrite(STATUS_LED_G, HIGH); // 绿灯表示连接成功
    Serial.println("BLE connected successfully!");
    
    // 发送初始状态
    sendSystemStatus();
}

void loop() {
    // 更新BLE连接状态
    bleComm.updateConnection();
    
    // 检查连接状态
    if (bleComm.isConnected()) {
        if (!digitalRead(STATUS_LED_G)) {
            digitalWrite(STATUS_LED_G, HIGH);
            Serial.println("BLE reconnected");
            sendSystemStatus(); // 重新连接时发送状态
        }
    } else {
        digitalWrite(STATUS_LED_G, LOW);
        digitalWrite(STATUS_LED_Y, HIGH);
        // 连接断开时停止电机
        motors.stopMotors();
    }
    
    // 其他周期性任务...
    delay(50);
}

// ============= 测试函数 =============
void testBleProtocol() {
    Serial.println("\n=== BLE Protocol Test ===");
    
    // 模拟BLE接收到的各种命令
    Serial.println("Testing motor control...");
    onBleMessage('c', "127,-127");  // 左轮前进，右轮后退
    delay(1000);
    
    Serial.println("Testing light control...");
    onBleMessage('l', "255,128");   // 前灯全亮，后灯半亮
    delay(1000);
    
    Serial.println("Testing indicator...");
    onBleMessage('i', "1,0");       // 左转向灯
    delay(1000);
    
    Serial.println("Testing heartbeat...");
    onBleMessage('h', "1000");      // 1秒心跳间隔
    delay(1000);
    
    Serial.println("Testing notification...");
    onBleMessage('n', "y,1");       // 黄色LED开
    delay(1000);
    
    Serial.println("Stopping motors...");
    onBleMessage('c', "0,0");       // 停止
    
    Serial.println("=== Test Complete ===\n");
} 