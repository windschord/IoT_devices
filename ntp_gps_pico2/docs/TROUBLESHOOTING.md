# GPS NTP Server - Troubleshooting Guide

## Overview

This guide provides comprehensive troubleshooting procedures for the GPS NTP Server system. Problems are organized by subsystem with systematic diagnostic approaches and resolution procedures.

## General Diagnostic Approach

### System Status Indicators

**LED Status Summary**:
- **Green (GPS)**: GPS satellite acquisition and time synchronization
- **Blue (Network)**: Ethernet network connectivity
- **Yellow (PPS)**: GPS PPS (Pulse Per Second) signal activity
- **Red (Error)**: System errors or critical failures

**Normal Operation Indicators**:
- Green: Solid on (satellites acquired)
- Blue: Solid on (network connected)
- Yellow: Flashing at 1Hz (PPS active)
- Red: Off (no errors)

### Web Interface Diagnostics

Access the web interface at `http://[device-ip]/` for:
- Real-time system status
- Configuration settings
- Error logs and diagnostics
- Performance metrics

### Serial Console Diagnostics

Connect to serial console (9600 baud) for detailed logging:
```bash
screen /dev/ttyUSB0 9600
```

## Power and Boot Issues

### System Won't Power On

**Symptoms**:
- No LED activity
- No serial output
- System completely unresponsive

**Diagnostic Steps**:
1. **Verify Power Supply**:
   - Check power adapter output voltage (5V ±5%)
   - Measure voltage at Pico 2 VSYS pin
   - Verify current capacity (minimum 500mA)

2. **Check Connections**:
   - Verify USB-C or power connector seating
   - Check for loose connections
   - Inspect for damaged cables

3. **Component Verification**:
   - Remove all peripheral modules
   - Test Pico 2 with minimal configuration
   - Check for short circuits on power rails

**Solutions**:
- Replace power adapter if voltage is incorrect
- Reseat all power connections
- Check for component failures with multimeter

### Boot Loop / Continuous Restart

**Symptoms**:
- LEDs flash repeatedly in startup pattern
- System restarts every 10-30 seconds
- Serial console shows repeated boot messages

**Diagnostic Steps**:
1. **Check Power Quality**:
   - Measure power supply ripple
   - Verify current capacity under load
   - Check for voltage drops during startup

2. **Component Issues**:
   - Disconnect modules one by one
   - Check I2C pull-up resistors (4.7kΩ)
   - Verify SPI connections to W5500

3. **Software Issues**:
   - Check for corrupted firmware
   - Verify configuration file integrity
   - Review error logs for failure patterns

**Solutions**:
- Upgrade power supply capacity
- Fix hardware connection issues
- Reflash firmware if corrupted
- Reset configuration to defaults

## GPS/GNSS Issues

### GPS Won't Acquire Satellites (Green LED Off)

**Symptoms**:
- Green LED remains off after 15+ minutes
- Web interface shows 0 satellites
- No PPS signal activity

**Diagnostic Steps**:
1. **Antenna Issues**:
   - Check antenna connection to ZED-F9T
   - Verify antenna placement (clear sky view)
   - Test with different antenna if available

2. **RF Environment**:
   - Check for nearby interference sources
   - Verify antenna is away from switching circuits
   - Test in different physical location

3. **Module Configuration**:
   - Verify I2C communication with GPS module
   - Check constellation settings
   - Review GPS module firmware version

**Solutions**:
```json
// Enable all constellations for better acquisition
{
  "gps": {
    "constellations": {
      "gps": true,
      "glonass": true,
      "galileo": true,
      "beidou": true,
      "qzss": true
    },
    "elevation_mask": 5
  }
}
```

- Move antenna to location with better sky view
- Check antenna cable for damage
- Replace antenna if defective
- Increase GPS update rate temporarily for faster acquisition

### Poor GPS Performance (Low Satellite Count)

**Symptoms**:
- GPS acquires but maintains <4 satellites
- Frequent loss of GPS lock
- High HDOP values (>5.0)

**Diagnostic Steps**:
1. **Signal Quality Analysis**:
   - Check individual satellite C/N0 ratios
   - Review satellite elevation angles
   - Monitor HDOP/VDOP trends

2. **Environmental Factors**:
   - Check for signal obstructions
   - Verify antenna gain specifications
   - Test at different times of day

**Solutions**:
- Relocate antenna for better coverage
- Upgrade to higher-gain active antenna
- Adjust constellation priorities:
```json
{
  "gps": {
    "constellation_priority": ["GPS", "Galileo", "GLONASS", "QZSS", "BeiDou"],
    "min_satellites": 4
  }
}
```

### PPS Signal Issues (Yellow LED Not Flashing)

**Symptoms**:
- GPS has satellite lock but no PPS signal
- Yellow LED remains off
- System falls back to Stratum 2/3

**Diagnostic Steps**:
1. **Hardware Verification**:
   - Check PPS connection (GPIO 8)
   - Verify wire integrity and connections
   - Measure PPS signal with oscilloscope if available

2. **GPS Module Configuration**:
   - Verify PPS output is enabled on ZED-F9T
   - Check PPS pulse width settings
   - Review timing mode configuration

**Solutions**:
```cpp
// Enable PPS output on ZED-F9T
myGNSS.setVal8(UBLOX_CFG_TP_PULSE_DEF, 1);  // Enable PPS
myGNSS.setVal32(UBLOX_CFG_TP_PERIOD_TP1, 1000000);  // 1 second period
myGNSS.setVal32(UBLOX_CFG_TP_LEN_TP1, 100000);      // 100ms pulse width
```

- Reconfigure PPS output settings
- Check GPIO configuration in firmware
- Verify PPS signal routing

## Network Connectivity Issues

### No Network Connection (Blue LED Off)

**Symptoms**:
- Blue LED remains off
- No IP address assigned
- Cannot access web interface

**Diagnostic Steps**:
1. **Physical Layer**:
   - Check Ethernet cable integrity
   - Verify cable is plugged into active network port
   - Test with known good cable

2. **Link Layer**:
   - Check W5500 SPI connections
   - Verify reset and interrupt signals
   - Test with different network switch/port

3. **Network Configuration**:
   - Check DHCP server availability
   - Verify network switch configuration
   - Test with static IP configuration

**Solutions**:
```json
// Configure static IP for testing
{
  "network": {
    "dhcp_enabled": false,
    "ip_address": "192.168.1.100",
    "netmask": "255.255.255.0",
    "gateway": "192.168.1.1"
  }
}
```

- Replace Ethernet cable
- Check network switch configuration
- Verify SPI connections to W5500
- Reset network configuration to defaults

### DHCP Issues

**Symptoms**:
- Blue LED flashes but no stable connection
- IP address not assigned
- Intermittent network connectivity

**Diagnostic Steps**:
1. **DHCP Server Issues**:
   - Check DHCP server logs
   - Verify DHCP scope has available addresses
   - Test DHCP with other devices

2. **MAC Address Issues**:
   - Check for MAC address conflicts
   - Verify MAC address filtering settings
   - Review DHCP reservations

**Solutions**:
- Configure DHCP reservation for device MAC
- Increase DHCP lease time
- Switch to static IP configuration
- Reset W5500 module

### Network Performance Issues

**Symptoms**:
- High NTP response times (>50ms)
- Packet loss to/from device
- Intermittent web interface access

**Diagnostic Steps**:
1. **Network Load**:
   - Monitor network utilization
   - Check for broadcast storms
   - Verify switch port speed/duplex

2. **Device Performance**:
   - Check system resource usage
   - Monitor NTP request rates
   - Review error counters

**Solutions**:
- Implement NTP rate limiting
- Upgrade network infrastructure
- Optimize system configuration

## NTP Service Issues

### NTP Server Not Responding

**Symptoms**:
- NTP clients cannot reach server
- Port 123 not accessible
- No NTP responses in logs

**Diagnostic Steps**:
1. **Service Status**:
   - Check if NTP service is enabled
   - Verify UDP port 123 is bound
   - Review firewall/ACL settings

2. **Network Connectivity**:
   - Test UDP connectivity to port 123
   - Check for firewall blocking
   - Verify routing to NTP clients

**Network Testing**:
```bash
# Test NTP connectivity from client
ntpdate -q [device-ip]

# Test UDP port 123
nc -u [device-ip] 123

# Monitor NTP packets
tcpdump -i any port 123
```

**Solutions**:
```json
// Ensure NTP service is enabled
{
  "ntp": {
    "enabled": true,
    "port": 123,
    "bind_all_interfaces": true
  }
}
```

### High NTP Response Times

**Symptoms**:
- NTP response times >10ms consistently
- Clients report high dispersion
- System health score degraded

**Diagnostic Steps**:
1. **System Load**:
   - Check CPU utilization
   - Monitor memory usage
   - Review interrupt latency

2. **Network Issues**:
   - Check for network congestion
   - Verify switch configuration
   - Monitor packet loss rates

**Solutions**:
- Reduce NTP client polling frequency
- Implement request rate limiting
- Optimize system configuration:
```json
{
  "ntp": {
    "rate_limit": {
      "requests_per_second": 10,
      "burst_limit": 20
    },
    "thread_priority": "high"
  }
}
```

### Stratum Issues

**Symptoms**:
- System reports incorrect stratum level
- Clients see stratum 16 (unsynchronized)
- Frequent stratum changes

**Diagnostic Steps**:
1. **GPS Synchronization**:
   - Verify GPS satellite lock
   - Check PPS signal activity
   - Review time accuracy

2. **Fallback Logic**:
   - Check RTC synchronization
   - Verify fallback thresholds
   - Review stratum calculation logic

**Stratum Calculation Logic**:
- **Stratum 1**: GPS synchronized with PPS active
- **Stratum 2**: GPS synchronized without PPS
- **Stratum 3**: GPS lost, using RTC fallback
- **Stratum 16**: System error or initialization

## Display Issues

### OLED Display Not Working

**Symptoms**:
- Display remains blank
- No startup messages
- Display backlight off

**Diagnostic Steps**:
1. **Hardware Issues**:
   - Check I2C connections (GPIO 0/1)
   - Verify 3.3V power to display
   - Test I2C pull-up resistors (4.7kΩ)

2. **I2C Communication**:
   - Scan I2C bus for device at 0x3C
   - Check for I2C conflicts with RTC
   - Verify I2C clock frequency

**I2C Diagnostic Commands**:
```cpp
// I2C bus scan
Wire.begin();
for (int addr = 1; addr < 127; addr++) {
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    Serial.print("Found device at 0x");
    Serial.println(addr, HEX);
  }
}
```

**Solutions**:
- Check and reseat I2C connections
- Verify pull-up resistors are installed
- Test with different display module
- Reset display configuration

### Display Shows Incorrect Information

**Symptoms**:
- Display updates but shows wrong data
- Garbled text or characters
- Display freezes on one screen

**Solutions**:
```json
// Reset display configuration
{
  "display": {
    "enabled": true,
    "brightness": 128,
    "contrast": 128,
    "flip_display": false,
    "refresh_rate": 1
  }
}
```

## System Integration Issues

### Multiple Component Failures

**Symptoms**:
- Multiple LEDs showing error states
- System health score <50%
- Web interface partially accessible

**Systematic Diagnosis**:
1. **Power System Check**:
   - Measure all voltage rails
   - Check current consumption
   - Verify ground connections

2. **I2C Bus Analysis**:
   - Check both I2C buses separately
   - Verify pull-up resistors
   - Test each device individually

3. **SPI Bus Analysis**:
   - Check W5500 SPI connections
   - Verify clock, data, and control signals
   - Test SPI bus integrity

**Recovery Procedure**:
1. Power cycle the system
2. Check all hardware connections
3. Reset configuration to defaults
4. Test components individually
5. Gradually enable features

### Performance Degradation

**Symptoms**:
- System health score declining over time
- Memory usage increasing
- Response times getting slower

**Diagnostic Approach**:
1. **Resource Monitoring**:
   - Track memory usage trends
   - Monitor CPU utilization
   - Check for memory leaks

2. **Performance Metrics**:
   - Review NTP response time trends
   - Check GPS acquisition performance
   - Monitor network throughput

**Solutions**:
- Restart system to clear memory leaks
- Optimize configuration settings
- Reduce logging verbosity
- Implement periodic maintenance procedures

## Advanced Diagnostics

### Firmware Recovery

**Complete System Failure**:
1. **Hardware Reset**:
   - Hold BOOTSEL button while connecting power
   - System enters USB mass storage mode
   - Drag new firmware file to mounted drive

2. **Configuration Reset**:
   - Delete configuration files from flash
   - System will recreate default configuration
   - Reconfigure system settings

### Professional Diagnostics

**For Complex Issues**:
1. **Logic Analyzer**: Monitor I2C/SPI communications
2. **Oscilloscope**: Analyze PPS signal timing
3. **Spectrum Analyzer**: Check RF environment
4. **Network Analyzer**: Monitor packet flows

## Support and Maintenance

### Regular Maintenance Schedule

**Daily**:
- Check LED status indicators
- Verify system time accuracy
- Monitor basic connectivity

**Weekly**:
- Review system logs
- Check performance metrics
- Verify backup procedures

**Monthly**:
- Comprehensive system testing
- Configuration backup
- Performance optimization review

**Quarterly**:
- Firmware update evaluation
- Hardware inspection
- Documentation updates

### Support Resources

**Documentation**:
- Hardware Setup Guide
- Configuration Guide
- API Reference Documentation

**Online Resources**:
- Project repository and issue tracker
- Community forums and discussions
- Technical support contacts

**Diagnostic Tools**:
- Web interface diagnostics
- Serial console logging
- Performance monitoring endpoints

### When to Contact Support

Contact technical support when:
- Hardware failure is suspected
- Multiple troubleshooting attempts have failed
- System safety or security concerns arise
- Custom configuration assistance is needed

**Information to Provide**:
- Complete system description and configuration
- Detailed symptom description
- Troubleshooting steps already attempted
- System logs and error messages
- Environmental conditions and setup details

## Conclusion

This troubleshooting guide covers the most common issues encountered with the GPS NTP Server system. For issues not covered here, systematic diagnosis following the general approach usually identifies the root cause. Remember to document any solutions discovered for future reference and to contribute back to the community knowledge base.