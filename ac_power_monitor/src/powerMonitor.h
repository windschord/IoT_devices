//
// Created by tsk on 2023/05/05.
//

#ifndef AC_POWER_MONITOR_POWERMONITOR_H
#define AC_POWER_MONITOR_POWERMONITOR_H

#include <Arduino.h>
#include <ModbusMaster.h>

class PowerMonitor {
private:
    ModbusMaster node;
    float voltage;
    float power;
    float current;
    uint32_t energy;
    float frequency;
    float powerFactor;
    bool alarmState;
    uint8_t pzemSlaveAddr;
public:
    void setup(uint8_t addr);

    void resetEnergy(uint8_t slaveAddr);

    void resetEnergy();

    void setAlarmThreshold(uint8_t slaveAddr, long alarmThreshold);

    void setAlarmThreshold(long alarmThreshold);

    void requestPowerData();

    float getAlarmThreshold();

    float getVoltage() {
        return voltage;
    }

    float getPower() {
        return power;
    }


    float getCurrent() {
        return current;
    };

    uint32_t getEnergy() {
        return energy;
    };

    float getFrequency() {
        return frequency;
    };

    float getPowerFactor() {
        return powerFactor;
    };

    bool getAlarmState() {
        return alarmState;
    };
};

#endif //AC_POWER_MONITOR_POWERMONITOR_H
