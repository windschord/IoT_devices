//
// Created by tsk on 2023/05/05.
//

#include <Arduino.h>
#include "powerMonitor.h"

void PowerMonitor::setup(uint8_t addr) {
    //setup Modbus
    pzemSlaveAddr = addr;
    Serial2.begin(9600, SERIAL_8N1);
    node.begin(addr, Serial2);
}

void PowerMonitor::resetEnergy(uint8_t slaveAddr) {
    //The command to reset the slave's energy is (total 4 bytes):
    //Slave address + 0x42 + CRC check high byte + CRC check low byte.
    uint16_t u16CRC = 0xFFFF;
    static uint8_t resetCommand = 0x42;
    u16CRC = crc16_update(u16CRC, slaveAddr);
    u16CRC = crc16_update(u16CRC, resetCommand);
    Serial.println("Resetting Energy");
    Serial2.write(slaveAddr);
    Serial2.write(resetCommand);
    Serial2.write(lowByte(u16CRC));
    Serial2.write(highByte(u16CRC));
    delay(1000);
}

void PowerMonitor::resetEnergy() {
    resetEnergy(pzemSlaveAddr);
}


void PowerMonitor::setAlarmThreshold(uint8_t slaveAddr, long alarmThreshold) {
    Serial.print("Set alarm threshold ");
    Serial.
            println(alarmThreshold);
    uint8_t result = node.writeSingleRegister(0x0001, alarmThreshold);
    if (result == node.ku8MBSuccess) {
        Serial.println("Write success!");
    } else {
        Serial.print("Write error: ");
        Serial.println(result);
    }
    delay(1000);
}

void PowerMonitor::setAlarmThreshold(long alarmThreshold) {
    setAlarmThreshold(pzemSlaveAddr, alarmThreshold);
}

void PowerMonitor::requestPowerData() {
    /*
    RegAddr Description                 Resolution
    0x0000  Voltage value               1LSB correspond to 0.1V
    0x0001  Current value low 16 bits   1LSB correspond to 0.001A
    0x0002  Current value high 16 bits
    0x0003  Power value low 16 bits     1LSB correspond to 0.1W
    0x0004  Power value high 16 bits
    0x0005  Energy value low 16 bits    1LSB correspond to 1Wh
    0x0006  Energy value high 16 bits
    0x0007  Frequency value             1LSB correspond to 0.1Hz
    0x0008  Power factor value          1LSB correspond to 0.01
    0x0009  Alarm status  0xFFFF is alarm，0x0000is not alarm
    */
    uint8_t result = node.readInputRegisters(0x0000, 10);  //read the 9 registers of the PZEM-014 / 016
    float *data = new float[10];

    if (result == node.ku8MBSuccess) {
        voltage = node.getResponseBuffer(0x0000) / 10.0;

        uint32_t tempdouble = 0x00000000;


        tempdouble |= node.getResponseBuffer(0x0003);       //LowByte
        tempdouble |= node.getResponseBuffer(0x0004) << 8;  //highByte
        power = tempdouble / 10.0;


        tempdouble = node.getResponseBuffer(0x0001);        //LowByte
        tempdouble |= node.getResponseBuffer(0x0002) << 8;  //highByte
        current = tempdouble / 1000.0;


        tempdouble = node.getResponseBuffer(0x0005);        //LowByte
        tempdouble |= node.getResponseBuffer(0x0006) << 8;  //highByte
        energy = tempdouble;

        frequency = node.getResponseBuffer(0x0007) / 10.0;
        powerFactor = node.getResponseBuffer(0x0008) / 100.0;

        alarmState = node.getResponseBuffer(0x0009) > 0;

//        Serial.print(voltage);
//        Serial.print("V   ");
//
//        Serial.print(current);
//        Serial.print("A   ");
//
//        Serial.print(power);
//        Serial.print("W  ");
//
//        Serial.print(powerFactor);
//        Serial.print("pf   ");
//
//        Serial.print(energy);
//        Serial.print("Wh  ");
//
//        Serial.print("ALM stat ");
//        Serial.print((alarm) ? "ALARM" : "OK");
//        Serial.println();
        node.clearResponseBuffer();

    } else {
        Serial.println("Failed to read modbus. Result: " + (String) result);
    }
}

float PowerMonitor::getAlarmThreshold() {
    /*
    RegAddr Description                 Resolution
    0x0001  Power alarm threshold       1LSB correspond to 1W
    */
    uint8_t result = node.readHoldingRegisters(0x0001, 1);
    float threshold = -1;

    if (result == node.ku8MBSuccess) {
        threshold = node.getResponseBuffer(0);

        Serial.println("ALM THL " + (String) threshold + "W");
        node.clearResponseBuffer();
    } else {
        Serial.println("Failed to read modbus. Result: " + (String) result);
    }
    return threshold;
}


