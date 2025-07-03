#include <OpenBotProtocol.h>
#include <MotorControl.h>
#include <BLEComm.h>
#include <WiFi.h>
#include <WiFiServer.h>

// ============= 硬件配置 =============
// 电机引脚配置
#define MOTOR_LEFT_PWM   5
#define MOTOR_LEFT_DIR1  18
#define MOTOR_LEFT_DIR2  19
#define MOTOR_RIGHT_PWM  16
#define MOTOR_RIGHT_DIR1 17
#define MOTOR_RIGHT_DIR2 4

// LED引脚配置
#define FRONT_LED_PIN    21
#define BACK_LED_PIN     22
#define STATUS_LED_Y     23
#define STATUS_LED_G     25
#define STATUS_LED_B     26

// 传感器引脚配置
#define SONAR_TRIG_PIN   12
#define SONAR_ECHO_PIN   14
#define VOLTAGE_PIN      A0

// ============= 网络配置 =============
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// ============= 对象实例 =============
OpenBotProtocol protocol;
MotorControl motors(MOTOR_LEFT_PWM, MOTOR_LEFT_DIR1, MOTOR_LEFT_DIR2,
                   MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIR1, MOTOR_RIGHT_DIR2);
BLEComm bleComm("OpenBot-ESP32");
WiFiServer server(8081);

// ============= 状态变量 =============
bool frontLedState = false;
bool backLedState = false;
bool leftIndicator = false;
bool rightIndicator = false;
unsigned long lastSensorRead = 0;
unsigned long lastHeartbeat = 0;
float batteryVoltage = 0.0;

// ============= 协议回调函数 =============

void onControlCommand(const ControlCommand& cmd) {
    Serial.printf("Motor Control - Left: %d, Right: %d\n", cmd.left, cmd.right);
    
    // 将协议命令转换为电机控制
    if (cmd.left == 0 && cmd.right == 0) {
        // 停止
        motors.stopMotors();
    } else if (cmd.left == cmd.right) {
        // 直线运动
        if (cmd.left > 0) {
            motors.moveForward(abs(cmd.left));
        } else {
            motors.moveBackward(abs(cmd.left));
        }
    } else if (cmd.left > cmd.right) {
        // 左转
        if (cmd.left > 0 && cmd.right >= 0) {
            motors.diagForwardLeft(abs(cmd.left));
        } else {
            motors.turnLeft(abs(cmd.left - cmd.right));
        }
    } else {
        // 右转
        if (cmd.right > 0 && cmd.left >= 0) {
            motors.diagForwardRight(abs(cmd.right));
        } else {
            motors.turnRight(abs(cmd.right - cmd.left));
        }
    }
    
    // 通过BLE发送状态反馈
    String status = "c" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(status.c_str());
}

void onLightCommand(const LightCommand& cmd) {
    Serial.printf("Light Control - Front: %d, Back: %d\n", cmd.front, cmd.back);
    
    // 控制前灯
    analogWrite(FRONT_LED_PIN, cmd.front);
    frontLedState = (cmd.front > 0);
    
    // 控制后灯
    analogWrite(BACK_LED_PIN, cmd.back);
    backLedState = (cmd.back > 0);
    
    // 状态反馈
    String status = "l" + String(cmd.front) + "," + String(cmd.back);
    bleComm.sendData(status.c_str());
}

void onIndicatorCommand(const IndicatorCommand& cmd) {
    Serial.printf("Indicator Control - Left: %d, Right: %d\n", cmd.left, cmd.right);
    
    leftIndicator = (cmd.left == 1);
    rightIndicator = (cmd.right == 1);
    
    // 这里可以添加指示灯闪烁逻辑
    // 在主循环中处理闪烁
    
    // 状态反馈
    String status = "i" + String(cmd.left) + "," + String(cmd.right);
    bleComm.sendData(status.c_str());
}

void onHeartbeatCommand(unsigned long interval) {
    Serial.printf("Heartbeat interval: %lu ms\n", interval);
    lastHeartbeat = millis();
    
    // 发送系统状态
    sendSystemStatus();
}

void onNotificationCommand(const NotificationCommand& cmd) {
    Serial.printf("Notification - LED: %c, State: %d\n", cmd.led, cmd.state);
    
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

void onJsonControlCommand(const JsonDriveCommand& cmd) {
    Serial.printf("JSON Drive Command - Left: %.2f, Right: %.2f\n", cmd.left, cmd.right);
    
    // 将浮点值转换为电机控制值
    int leftMotor, rightMotor;
    OpenBotProtocol::floatToSerialRange(cmd.left, leftMotor);
    OpenBotProtocol::floatToSerialRange(cmd.right, rightMotor);
    
    // 创建控制命令并处理
    ControlCommand motorCmd = {leftMotor, rightMotor};
    onControlCommand(motorCmd);
}

void onJsonCommand(const char* command) {
    Serial.printf("JSON Command: %s\n", command);
    
    // 处理各种JSON命令
    if (strcmp(command, "NOISE") == 0) {
        // 切换蜂鸣器或音效
        Serial.println("Noise command received");
    } else if (strcmp(command, "LOGS") == 0) {
        // 切换日志记录
        Serial.println("Logs command received");
    } else if (strcmp(command, "INDICATOR_LEFT") == 0) {
        IndicatorCommand indCmd = {1, 0};
        onIndicatorCommand(indCmd);
    } else if (strcmp(command, "INDICATOR_RIGHT") == 0) {
        IndicatorCommand indCmd = {0, 1};
        onIndicatorCommand(indCmd);
    } else if (strcmp(command, "INDICATOR_STOP") == 0) {
        IndicatorCommand indCmd = {0, 0};
        onIndicatorCommand(indCmd);
    } else if (strcmp(command, "NETWORK") == 0) {
        // 网络模式切换
        sendSystemStatus();
    } else if (strcmp(command, "DRIVE_MODE") == 0) {
        // 驱动模式切换
        Serial.println("Drive mode command received");
    }
}

// ============= BLE消息处理 =============
void onBleMessage(char header, const char* body) {
    // 将BLE消息转发给协议处理器
    String message = String(header) + String(body);
    protocol.processSerialMessage(message.c_str());
}

// ============= 辅助函数 =============

void sendSystemStatus() {
    // 发送电池电压
    readBatteryVoltage();
    protocol.sendSensorData("v", batteryVoltage);
    
    // 发送声纳距离
    float distance = readSonarDistance();
    if (distance > 0) {
        protocol.sendSensorData("s", distance);
    }
    
    // 发送系统特性
    protocol.sendFeatureResponse("ESP32:v:i:s:wf:lf:lb:ls:");
}

void readBatteryVoltage() {
    int rawValue = analogRead(VOLTAGE_PIN);
    // 假设使用电压分压器，比例为 (20k + 10k) / 10k = 3
    batteryVoltage = (rawValue * 3.3 * 3.0) / 4096.0;
}

float readSonarDistance() {
    // 触发声纳
    digitalWrite(SONAR_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(SONAR_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SONAR_TRIG_PIN, LOW);
    
    // 读取回声
    long duration = pulseIn(SONAR_ECHO_PIN, HIGH, 30000); // 30ms超时
    if (duration == 0) return -1; // 超时
    
    // 计算距离 (cm)
    float distance = duration * 0.034 / 2;
    return distance;
}

void handleIndicators() {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    
    if (millis() - lastBlink > 500) { // 500ms闪烁间隔
        lastBlink = millis();
        blinkState = !blinkState;
        
        // 处理左指示灯
        if (leftIndicator) {
            // 这里添加左指示灯闪烁逻辑
            // 例如：digitalWrite(LEFT_INDICATOR_PIN, blinkState);
        }
        
        // 处理右指示灯
        if (rightIndicator) {
            // 这里添加右指示灯闪烁逻辑
            // 例如：digitalWrite(RIGHT_INDICATOR_PIN, blinkState);
        }
    }
}

void checkConnection() {
    if (!protocol.isConnected() && (millis() - lastHeartbeat > 5000)) {
        // 连接超时，停止所有运动
        motors.stopMotors();
        analogWrite(FRONT_LED_PIN, 0);
        analogWrite(BACK_LED_PIN, 0);
        Serial.println("Connection timeout - stopping all motors");
    }
}

// ============= 主程序 =============

void setup() {
    Serial.begin(115200);
    
    // 初始化引脚
    pinMode(FRONT_LED_PIN, OUTPUT);
    pinMode(BACK_LED_PIN, OUTPUT);
    pinMode(STATUS_LED_Y, OUTPUT);
    pinMode(STATUS_LED_G, OUTPUT);
    pinMode(STATUS_LED_B, OUTPUT);
    pinMode(SONAR_TRIG_PIN, OUTPUT);
    pinMode(SONAR_ECHO_PIN, INPUT);
    pinMode(VOLTAGE_PIN, INPUT);
    
    // 初始化模块
    motors.init();
    bleComm.init();
    protocol.init();
    
    // 设置协议回调
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    protocol.setNotificationCallback(onNotificationCommand);
    protocol.setJsonControlCallback(onJsonControlCommand);
    protocol.setJsonCommandCallback(onJsonCommand);
    
    // 设置BLE回调
    bleComm.setMessageCallback(onBleMessage);
    
    // 连接WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        digitalWrite(STATUS_LED_Y, !digitalRead(STATUS_LED_Y)); // 黄灯闪烁表示连接中
    }
    
    digitalWrite(STATUS_LED_Y, LOW);
    digitalWrite(STATUS_LED_G, HIGH); // 绿灯表示WiFi连接成功
    
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // 启动服务器
    server.begin();
    Serial.println("Server started on port 8081");
    
    // 指示系统就绪
    for (int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_B, HIGH);
        delay(200);
        digitalWrite(STATUS_LED_B, LOW);
        delay(200);
    }
    
    Serial.println("System ready!");
    lastHeartbeat = millis();
}

void loop() {
    // 更新BLE连接状态
    bleComm.updateConnection();
    
    // 处理串口数据（来自USB或其他串口设备）
    while (Serial.available()) {
        char data = Serial.read();
        protocol.processSerialData(data);
    }
    
    // 处理网络客户端（JSON协议）
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected");
        
        while (client.connected()) {
            if (client.available()) {
                String jsonMessage = client.readStringUntil('\n');
                jsonMessage.trim();
                
                if (jsonMessage.length() > 0) {
                    Serial.print("Received JSON: ");
                    Serial.println(jsonMessage);
                    
                    // 解析JSON消息
                    protocol.parseJsonMessage(jsonMessage.c_str());
                    
                    // 发送确认响应
                    client.println("OK");
                }
            }
            
            // 继续处理其他任务
            bleComm.updateConnection();
            handleIndicators();
            
            // 处理串口数据
            while (Serial.available()) {
                char data = Serial.read();
                protocol.processSerialData(data);
            }
            
            delay(10);
        }
        
        client.stop();
        Serial.println("Client disconnected");
    }
    
    // 定期任务
    if (millis() - lastSensorRead > 1000) { // 每秒读取一次传感器
        lastSensorRead = millis();
        
        // 读取并发送传感器数据
        readBatteryVoltage();
        float distance = readSonarDistance();
        
        if (bleComm.isConnected()) {
            String sensorData = "v" + String(batteryVoltage, 2);
            bleComm.sendData(sensorData.c_str());
            
            if (distance > 0) {
                sensorData = "s" + String(distance, 1);
                bleComm.sendData(sensorData.c_str());
            }
        }
    }
    
    // 处理指示灯闪烁
    handleIndicators();
    
    // 检查连接状态
    checkConnection();
    
    delay(10);
} 