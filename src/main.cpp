#include <Arduino.h>
#include <USBCDC.h>
#include "MotorControl.h"
#include "SensorControl.h"
#include "BLEComm.h"
#include "../lib/protocol/lib/OpenBotProtocol.h"

#if ENABLE_WIFI_DEBUG
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#endif

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

#if ENABLE_WIFI_DEBUG
// WiFi配置 - 请根据实际环境修改
const char* wifi_ssid = "TESLA";     
const char* wifi_password = "12344321a";
WebServer server(80);
String lastCommand = "";
String lastResult = "";
#endif

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

// Light command callback (l)
void onLightCommand(const LightCommand& cmd) {
    Serial.printf("Light Command - Front: %d, Back: %d\n", cmd.front, cmd.back);
    // TODO: Control actual lights here
}

// Indicator command callback (i)
void onIndicatorCommand(const IndicatorCommand& cmd) {
    Serial.printf("Indicator Command - Left: %d, Right: %d\n", cmd.left, cmd.right);
    // TODO: Control indicator GPIOs here
}

// Notification command callback (n)
void onNotificationCommand(const NotificationCommand& cmd) {
    Serial.printf("Notification Command - LED: %c, State: %d\n", cmd.led, cmd.state);
    // TODO: Control notification LED
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
    
#if ENABLE_WIFI_DEBUG
    // 保存响应供Web界面使用
    lastResult += String(data);
#endif
    
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

#if ENABLE_WIFI_DEBUG
// WiFi debug functions

void setupWebServer() {
    // 主页面 - 使用英文避免UTF-8编译问题
    server.on("/", HTTP_GET, []() {
        String html = "<!DOCTYPE html><html><head><title>OpenBot ESP32 Debug</title><style>";
        html += "body{font-family:Arial,sans-serif;margin:40px;background:#f5f5f5;}";
        html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
        html += "h1{color:#333;text-align:center;}";
        html += ".form-group{margin:20px 0;}";
        html += "label{display:block;margin-bottom:5px;font-weight:bold;}";
        html += "input[type='text']{width:100%;padding:10px;border:1px solid #ddd;border-radius:4px;font-size:16px;}";
        html += "button{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;font-size:16px;}";
        html += "button:hover{background:#0056b3;}";
        html += ".result{margin-top:20px;padding:15px;background:#f8f9fa;border:1px solid #e9ecef;border-radius:4px;}";
        html += ".examples{margin-top:20px;}";
        html += ".examples h3{color:#666;}";
        html += ".examples ul{list-style-type:none;padding:0;}";
        html += ".examples li{background:#e7f3ff;padding:8px;margin:5px 0;border-radius:4px;cursor:pointer;}";
        html += ".examples li:hover{background:#d1ecf1;}";
        html += "</style></head><body>";
        html += "<div class='container'>";
        html += "<h1>OpenBot ESP32 Debug Interface</h1>";
        html += "<div class='form-group'>";
        html += "<label for='command'>Enter Protocol Command:</label>";
        html += "<input type='text' id='command' placeholder='e.g: c100,50 or l255,128' />";
        html += "</div>";
        html += "<button onclick='sendCommand()'>Send Command</button>";
        html += "<div class='result' id='result'><strong>Result:</strong> Waiting for command...</div>";
        html += "<div class='examples'>";
        html += "<h3>Example Commands (click to fill):</h3>";
        html += "<ul>";
        html += "<li onclick='fillCommand(\"c100,50\")'>c100,50 - Motor Control (Left:100, Right:50)</li>";
        html += "<li onclick='fillCommand(\"c0,0\")'>c0,0 - Stop Motors</li>";
        html += "<li onclick='fillCommand(\"c-100,100\")'>c-100,100 - Turn Left in Place</li>";
        html += "<li onclick='fillCommand(\"l255,128\")'>l255,128 - LED Control (Front:255, Back:128)</li>";
        html += "<li onclick='fillCommand(\"i1,0\")'>i1,0 - Indicator Control (Left:On, Right:Off)</li>";
        html += "<li onclick='fillCommand(\"h500\")'>h500 - Set Heartbeat Interval to 500ms</li>";
        html += "<li onclick='fillCommand(\"f\")'>f - Query Features</li>";
        html += "<li onclick='fillCommand(\"v1000\")'>v1000 - Set Voltage Query Interval to 1000ms</li>";
        html += "</ul></div></div>";
        html += "<script>";
        html += "function fillCommand(cmd){document.getElementById('command').value=cmd;}";
        html += "function sendCommand(){";
        html += "const command=document.getElementById('command').value;";
        html += "if(!command){alert('Please enter a command');return;}";
        html += "fetch('/execute',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({command:command})})";
        html += ".then(response=>response.json())";
        html += ".then(data=>{";
        html += "const resultDiv=document.getElementById('result');";
        html += "resultDiv.innerHTML='<strong>Command:</strong> '+data.command+'<br><strong>Status:</strong> '+data.status+'<br><strong>Response:</strong> '+(data.response||'No response')+'<br><strong>Time:</strong> '+new Date().toLocaleString();";
        html += "})";
        html += ".catch(error=>{document.getElementById('result').innerHTML='<strong>Error:</strong> '+error.message;});";
        html += "}";
        html += "document.getElementById('command').addEventListener('keypress',function(e){if(e.key==='Enter'){sendCommand();}});";
        html += "</script></body></html>";
        
        server.send(200, "text/html", html);
    });
    
    // Execute command API
    server.on("/execute", HTTP_POST, []() {
        if (server.hasArg("plain")) {
            String body = server.arg("plain");
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, body);
            
            String command = doc["command"];
            lastCommand = command;
            lastResult = "";
            
            // Execute command
            Serial.printf("WiFi Debug: Executing command: %s\n", command.c_str());
            protocol.processSerialMessage(command.c_str());
            
            // Wait for processing result
            delay(100);
            
            // Return result
            DynamicJsonDocument response(1024);
            response["command"] = command;
            response["status"] = "executed";
            response["response"] = lastResult.length() > 0 ? lastResult : "command sent";
            
            String responseStr;
            serializeJson(response, responseStr);
            server.send(200, "application/json", responseStr);
        } else {
            server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
        }
    });
    
    server.begin();
    Serial.println("Web server started on port 80");
}

void initWiFi() {
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.print("Connecting to WiFi");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi connected! IP address: ");
        Serial.println(WiFi.localIP());
        setupWebServer();
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

void handleWebServer() {
    server.handleClient();
}
#endif

// Test protocol parsing
void testProtocolParsing() {
    Serial.println("\n=== Protocol Parsing Test ===");
    
    // Test control commands
    Serial.println("Testing: c100,-50");
    protocol.processSerialMessage("c100,-50");
    
    Serial.println("Testing: c0,0");
    protocol.processSerialMessage("c0,0");
    
    Serial.println("Testing: l255,128");
    protocol.processSerialMessage("l255,128");
    
    Serial.println("=== Test Complete ===\n");
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    Serial.println('r');

    // Initialize motor control
    motors.init();

    // Initialize sensors
    // sensors.init();

    // Initialize protocol processor and register callbacks
    protocol.init();
    protocol.setControlCallback(onControlCommand);
    protocol.setLightCallback(onLightCommand);
    protocol.setIndicatorCallback(onIndicatorCommand);
    protocol.setHeartbeatCallback(onHeartbeatCommand);
    protocol.setNotificationCallback(onNotificationCommand);
    protocol.setSendCallback(onSendData);

#if (HAS_BLUETOOTH)
    // Initialize Bluetooth
    ble.init();
    ble.setMessageCallback(handleMessage);
#endif

#if ENABLE_WIFI_DEBUG
    // Initialize WiFi debugging
    Serial.println("Initializing WiFi debug...");
    initWiFi();
#endif

    // Wait 3 seconds then test protocol parsing
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

#if ENABLE_WIFI_DEBUG
    // Handle WiFi debug web requests
    handleWebServer();
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