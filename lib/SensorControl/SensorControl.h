#ifndef SENSOR_CONTROL_H
#define SENSOR_CONTROL_H

#include <Arduino.h>

class SensorControl {
public:
    // 构造函数
    SensorControl(
        uint16_t triggerPin,
        uint16_t echoPin,
        uint16_t vinPin,
        uint16_t ledLeftPin,
        uint16_t ledRightPin
    );

    // 初始化函数
    void init();

    // 超声波相关函数
    void startPing();
    bool checkEcho();
    unsigned int getDistance();
    void updateDistanceEstimate();

    // 电压监测相关函数
    void updateVoltage();
    float getVoltage();
    bool isLowBattery();

    // LED控制函数
    void setLeftLED(bool state);
    void setRightLED(bool state);

    // 配置函数
    void setSonarInterval(unsigned long interval);
    void setVoltageInterval(unsigned long interval);

private:
    // 引脚定义
    uint16_t _triggerPin;
    uint16_t _echoPin;
    uint16_t _vinPin;
    uint16_t _ledLeftPin;
    uint16_t _ledRightPin;

    // 超声波相关变量
    const float US_TO_CM = 0.01715;
    const unsigned int MAX_SONAR_DISTANCE = 300;
    const unsigned long MAX_SONAR_TIME = (long)MAX_SONAR_DISTANCE * 2 * 10 / 343 + 1;
    unsigned long _sonarInterval = 1000;
    unsigned long _sonarTime = 0;
    bool _sonarSent = false;
    bool _pingSuccess = false;
    unsigned int _distance = -1;
    unsigned int _distanceEstimate = -1;
    unsigned long _startTime;
    unsigned long _echoTime = 0;

    // 电压监测相关变量
    const float VOLTAGE_DIVIDER_FACTOR = (30 + 10) / 10;
    const float VOLTAGE_MIN = 6.0f;
    const float VOLTAGE_LOW = 9.0f;
    const float VOLTAGE_MAX = 12.6f;
    const float ADC_FACTOR = 3.3 / 4095;
    unsigned int _vinCounter = 0;
    const unsigned int _vinArraySz = 10;
    int _vinArray[10] = {0};
    unsigned long _voltageInterval = 1000;
    unsigned long _voltageTime = 0;
};

#endif // SENSOR_CONTROL_H 