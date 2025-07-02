#include <OpenBotProtocol.h>
#include <WiFi.h>
#include <WiFiServer.h>

// WiFi配置
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// 创建协议处理器实例
OpenBotProtocol protocol;

// WiFi服务器（用于网络协议）
WiFiServer server(8081);

// 回调函数实现
void onControlCommand(const ControlCommand& cmd) {
    Serial.print("Motor Control - Left: ");
    Serial.print(cmd.left);
    Serial.print(", Right: ");
    Serial.println(cmd.right);
    
    // 在这里添加您的电机控制代码
    // 例如：controlMotors(cmd.left, cmd.right);
}

void onLightCommand(const LightCommand& cmd) {
    Serial.print("Light Control - Front: ");
    Serial.print(cmd.front);
    Serial.print(", Back: ");
    Serial.println(cmd.back);
    
    // 在这里添加您的LED控制代码
}

void onIndicatorCommand(const IndicatorCommand& cmd) {
    Serial.print("Indicator Control - Left: ");
    Serial.print(cmd.left);
    Serial.print(", Right: ");
    Serial.println(cmd.right);
    
    // 在这里添加您的指示灯控制代码
}

void onHeartbeatCommand(unsigned long interval) {
    Serial.print("Heartbeat interval: ");
    Serial.println(interval);
    
    // 心跳处理逻辑
}

void onNotificationCommand(const NotificationCommand& cmd) {
    Serial.print("Notification - LED: ");
    Serial.print(cmd.led);
    Serial.print(", State: ");
    Serial.println(cmd.state);
    
    // 状态LED控制代码
}

void onJsonControlCommand(const JsonDriveCommand& cmd) {
    Serial.print("JSON Drive Command - Left: ");
    Serial.print(cmd.left);
    Serial.print(", Right: ");
    Serial.println(cmd.right);
    
    // 将浮点值转换为电机控制值
    int leftMotor, rightMotor;
    OpenBotProtocol::floatToSerialRange(cmd.left, leftMotor);
    OpenBotProtocol::floatToSerialRange(cmd.right, rightMotor);
    
    // 调用电机控制函数
    // controlMotors(leftMotor, rightMotor);
}

void onJsonCommand(const char* command) {
    Serial.print("JSON Command: ");
    Serial.println(command);
    
    // 处理各种命令
    if (strcmp(command, "NOISE") == 0) {
        // 处理NOISE命令
    } else if (strcmp(command, "LOGS") == 0) {
        // 处理LOGS命令
    } else if (strcmp(command, "INDICATOR_RIGHT") == 0) {
        // 处理右转向灯命令
    } else if (strcmp(command, "INDICATOR_LEFT") == 0) {
        // 处理左转向灯命令
    } else if (strcmp(command, "INDICATOR_STOP") == 0) {
        // 处理停止转向灯命令
    }
}

void setup() {
    Serial.begin(115200);
    
    // 初始化协议处理器
    protocol.init();
    
    // 设置回调函数
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    protocol.setNotificationCallback(onNotificationCommand);
    protocol.setJsonControlCallback(onJsonControlCommand);
    protocol.setJsonCommandCallback(onJsonCommand);
    
    // 连接WiFi（用于网络协议）
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // 启动服务器
    server.begin();
    Serial.println("Server started on port 8081");
    
    Serial.println("System ready!");
}

void loop() {
    // 处理串口数据（Arduino协议）
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
                }
            }
            
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
    
    // 检查连接状态
    if (!protocol.isConnected()) {
        // 连接超时处理
        // Serial.println("Connection timeout");
    }
    
    delay(10);
} 