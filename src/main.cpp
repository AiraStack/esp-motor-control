#include <Arduino.h>
#include "MotorControl.h"

// 电机引脚定义
const uint16_t PWMA = 25;         
const uint16_t AIN2 = 17;        
const uint16_t AIN1 = 21;         
const uint16_t BIN1 = 22;       
const uint16_t BIN2 = 23;        
const uint16_t PWMB = 26;   

// 创建MotorControl对象
MotorControl motors(PWMA, AIN1, AIN2, PWMB, BIN1, BIN2);

// 动作序列计数器
int actionCounter = 0;
unsigned long lastActionTime = 0;
const unsigned long actionDuration = 2000; // 每个动作持续2秒

void setup() {
  Serial.begin(115200);
  motors.init();
  Serial.println("Motor control initialized");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 每隔actionDuration毫秒更换一个动作
  if (currentTime - lastActionTime >= actionDuration) {
    lastActionTime = currentTime;
    actionCounter = (actionCounter + 1) % 10; // 10种不同的动作
    
    Serial.print("执行动作序列: ");
    Serial.println(actionCounter);
    
    // 使用库中的performActionSequence方法执行动作
    motors.performActionSequence(actionCounter);
  }
} 