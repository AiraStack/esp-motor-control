# MotorControl (中文)

这是一个用于ESP32的机器人底盘电机控制库，为OpenBot项目开发。该库抽象了小车的基本动作，使开发者能够轻松地控制机器人的移动。

## 功能特点

- 简单易用的电机控制接口
- 支持基本动作：前进、后退、左转、右转、原地旋转等
- 支持高级动作：斜向移动、预设动作序列等
- 使用ESP32的LEDC功能实现PWM控制

## 硬件要求

- ESP32开发板
- H桥电机驱动器（如L298N、TB6612等）
- 直流电机（2个）

## 引脚配置

默认引脚配置如下，但可以在创建MotorControl对象时自定义：

| 功能  | ESP32引脚 |
|------|----------|
| PWMA | 25       |
| AIN1 | 21       |
| AIN2 | 17       |
| BIN1 | 22       |
| BIN2 | 23       |
| PWMB | 26       |

## 安装

1. 在PlatformIO项目中，将此库复制到`lib`目录
2. 在代码中引入头文件：`#include "MotorControl.h"`

## 使用方法

### 基本使用

```cpp
#include <Arduino.h>
#include "MotorControl.h"

// 创建MotorControl对象，使用默认引脚配置
MotorControl motors(25, 21, 17, 26, 22, 23);

void setup() {
  // 初始化电机
  motors.init();
}

void loop() {
  // 前进
  motors.moveForward(100);
  delay(2000);
  
  // 停止
  motors.stopMotors();
  delay(1000);
  
  // 后退
  motors.moveBackward(100);
  delay(2000);
}
```

### 高级使用

查看`examples`目录中的示例代码以了解更多用法。

## API参考

### 构造函数

```cpp
MotorControl(uint16_t pwmA, uint16_t ain1, uint16_t ain2,
             uint16_t pwmB, uint16_t bin1, uint16_t bin2,
             int freq = 100000, uint16_t resolution = 8);
```

### 基本方法

- `void init()` - 初始化电机控制
- `void moveForward(uint16_t speed)` - 前进
- `void moveBackward(uint16_t speed)` - 后退
- `void turnLeft(uint16_t speed)` - 左转
- `void turnRight(uint16_t speed)` - 右转
- `void spinLeft(uint16_t speed)` - 原地左转
- `void spinRight(uint16_t speed)` - 原地右转
- `void stopMotors()` - 停止所有电机

### 高级方法

- `void diagForwardLeft(uint16_t speed)` - 斜向左前进
- `void diagForwardRight(uint16_t speed)` - 斜向右前进
- `void performActionSequence(int actionNumber)` - 执行预设动作序列

## 许可证

MIT 