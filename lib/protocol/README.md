# OpenBot ESP小车控制协议整理

本目录包含从OpenBot项目中整理出的ESP小车控制协议相关内容。

## 📁 目录结构

```
protocol/
├── README.md                   # 本文件
├── lib/                        # C++协议库
│   ├── OpenBotProtocol.h       # 协议库头文件
│   ├── OpenBotProtocol.cpp     # 协议库实现
│   ├── library.json            # PlatformIO库配置
│   └── examples/               # 使用示例
│       └── BasicUsage/
│           └── BasicUsage.ino  # 基本使用示例
└── docs/                       # 协议设计文档
    └── protocol_design.md      # 详细协议设计文档
```

## 🎯 协议特性

### 支持的协议类型

1. **串口协议** - 用于与Android应用通信
   - 简单的文本格式
   - 高效的解析速度
   - 低内存占用

2. **JSON协议** - 用于网络通信
   - 结构化数据格式
   - 易于扩展
   - 跨平台兼容

### 支持的命令类型

- ✅ 电机控制 (`c`)
- ✅ LED灯光控制 (`l`)
- ✅ 指示灯控制 (`i`)
- ✅ 心跳检测 (`h`)
- ✅ 功能查询 (`f`)
- ✅ 电压监测 (`v`)
- ✅ 传感器控制 (`s`, `b`, `w`)
- ✅ 状态通知 (`n`)

## 🚀 快速开始

### 1. 安装依赖

在`platformio.ini`中添加：

```ini
lib_deps = 
    bblanchon/ArduinoJson@^6.21.0
```

### 2. 包含库文件

将`protocol/lib/`目录复制到您的项目中，或添加为库依赖。

### 3. 基本使用

```cpp
#include <OpenBotProtocol.h>

OpenBotProtocol protocol;

void onControlCommand(const ControlCommand& cmd) {
    // 控制电机
    Serial.printf("Motor: L=%d, R=%d\n", cmd.left, cmd.right);
}

void setup() {
    protocol.init();
    protocol.setControlCallback(onControlCommand);
}

void loop() {
    // 处理串口数据
    while (Serial.available()) {
        protocol.processSerialData(Serial.read());
    }
}
```

## 📖 协议格式

### 串口协议示例

```
c127,-127\n          # 左轮前进，右轮后退
l255,0\n             # 前灯全亮，后灯关闭
i1,0\n               # 左转向灯开
```

### JSON协议示例

```json
{"driveCmd": {"l": 1.0, "r": -1.0}}  // 驱动命令
{"command": "INDICATOR_LEFT"}         // 功能命令
```

## 🔧 与现有库集成

该协议库可以与现有的`MotorControl`和`BLEComm`库无缝集成：

```cpp
#include <OpenBotProtocol.h>
#include <MotorControl.h>

OpenBotProtocol protocol;
MotorControl motors(5, 18, 19, 16, 17, 4);

void onControlCommand(const ControlCommand& cmd) {
    // 使用现有的MotorControl库
    if (cmd.left == cmd.right) {
        if (cmd.left > 0) motors.moveForward(cmd.left);
        else if (cmd.left < 0) motors.moveBackward(-cmd.left);
        else motors.stopMotors();
    } else {
        // 转向逻辑
        // ...
    }
}
```

## 📚 详细文档

更多详细信息请参阅：
- [协议设计文档](docs/protocol_design.md) - 完整的协议规范和设计说明
- [使用示例](lib/examples/BasicUsage/BasicUsage.ino) - 完整的使用示例代码

## 🤝 贡献

欢迎提交Issue和Pull Request来改进这个协议库！

## 📄 许可证

MIT License - 详见OpenBot项目的许可证条款。

---

**基于**: OpenBot项目 (https://github.com/isl-org/OpenBot)  
**整理者**: AiraStack Team 