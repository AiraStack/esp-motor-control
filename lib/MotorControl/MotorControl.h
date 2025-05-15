#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

class MotorControl {
public:
    // 构造函数
    MotorControl(
        uint16_t pwmA, uint16_t ain1, uint16_t ain2,
        uint16_t pwmB, uint16_t bin1, uint16_t bin2,
        int freq = 100000, uint16_t resolution = 8
    );
    
    // 初始化函数
    void init();
    
    // 基本动作接口
    void moveForward(uint16_t speed);
    void moveBackward(uint16_t speed);
    void turnLeft(uint16_t speed);
    void turnRight(uint16_t speed);
    void spinLeft(uint16_t speed);
    void spinRight(uint16_t speed);
    void stopMotors();
    
    // 高级动作接口
    void diagForwardLeft(uint16_t speed);
    void diagForwardRight(uint16_t speed);
    
    // 简单动作序列
    void performActionSequence(int actionNumber);

private:
    // 引脚定义
    uint16_t _pwmA, _ain1, _ain2;
    uint16_t _pwmB, _bin1, _bin2;
    
    // PWM配置
    int _freq;
    int _channelA;
    int _channelB;
    uint16_t _resolution;
    
    // 单个电机控制函数
    void forwardA(uint16_t pwm);
    void forwardB(uint16_t pwm);
    void backwardA(uint16_t pwm);
    void backwardB(uint16_t pwm);
    void stopA();
    void stopB();
};

#endif // MOTOR_CONTROL_H 