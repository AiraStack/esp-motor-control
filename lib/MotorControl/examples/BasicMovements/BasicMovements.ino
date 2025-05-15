#include <Arduino.h>
#include "MotorControl.h"

// ESP32引脚定义
const uint16_t PWMA = 25;         
const uint16_t AIN2 = 17;        
const uint16_t AIN1 = 21;         
const uint16_t BIN1 = 22;       
const uint16_t BIN2 = 23;        
const uint16_t PWMB = 26;   

// 创建电机控制对象
MotorControl motors(PWMA, AIN1, AIN2, PWMB, BIN1, BIN2);

void setup() {
  Serial.begin(115200);
  
  // 初始化电机
  motors.init();
  
  Serial.println("MotorControl库 - 基本动作演示");
  Serial.println("----------------------------");
  delay(1000);
}

void loop() {
  // 基本动作演示
  Serial.println("1. 前进");
  motors.moveForward(100);
  delay(2000);
  
  Serial.println("2. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("3. 后退");
  motors.moveBackward(100);
  delay(2000);
  
  Serial.println("4. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("5. 左转");
  motors.turnLeft(100);
  delay(2000);
  
  Serial.println("6. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("7. 右转");
  motors.turnRight(100);
  delay(2000);
  
  Serial.println("8. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("9. 原地左转");
  motors.spinLeft(100);
  delay(2000);
  
  Serial.println("10. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("11. 原地右转");
  motors.spinRight(100);
  delay(2000);
  
  Serial.println("12. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("13. 斜向左前进");
  motors.diagForwardLeft(100);
  delay(2000);
  
  Serial.println("14. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("15. 斜向右前进");
  motors.diagForwardRight(100);
  delay(2000);
  
  Serial.println("16. 停止");
  motors.stopMotors();
  delay(1000);
  
  Serial.println("演示完成，重新开始...");
  delay(3000);
} 