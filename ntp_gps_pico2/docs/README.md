# GPS NTP Server - Documentation Overview

## Project Overview

The GPS NTP Server is a high-precision Network Time Protocol (NTP) server implementation based on Raspberry Pi Pico 2, providing stratum-1 time services synchronized to GPS/GNSS satellites with PPS (Pulse Per Second) accuracy.

## System Architecture

### Hardware Platform
- **Main Controller**: Raspberry Pi Pico 2 (RP2350 dual-core ARM Cortex-M33)
- **GPS/GNSS Receiver**: SparkFun GNSS Timing Breakout ZED-F9T
- **Network Interface**: W5500 Ethernet Controller
- **Display**: SH1106 OLED (128x64 pixels)
- **Backup Time**: DS3231 Real-Time Clock

### Key Features
- **Multi-Constellation GNSS**: GPS, GLONASS, Galileo, BeiDou, QZSS support
- **High Precision**: PPS signal synchronization for microsecond accuracy
- **QZSS L1S Support**: Japan disaster and crisis management alerts
- **Network Time Service**: RFC 5905 compliant NTPv4 server
- **Web Management**: HTTP configuration interface
- **Monitoring**: Prometheus metrics and Syslog support
- **Fault Tolerance**: Automatic GPS/RTC failover

## Documentation Structure

### 📚 User Guides

#### [Hardware Setup Guide](HARDWARE_SETUP.md)
Complete instructions for assembling the GPS NTP Server system:
- **Component Requirements**: Detailed parts list and specifications
- **Wiring Diagrams**: Pin assignments and connection details
- **Assembly Instructions**: Step-by-step hardware construction
- **Power Requirements**: Power consumption analysis and supply options
- **Signal Quality**: Antenna placement and RF considerations
- **Enclosure Design**: Indoor and outdoor installation recommendations
- **Testing Procedures**: Hardware verification and troubleshooting

#### [Configuration Guide](CONFIGURATION.md)
Software configuration and operational procedures:
- **Initial Setup**: First boot and network configuration
- **Web Interface**: Browser-based configuration management
- **Network Configuration**: DHCP, static IP, and security settings
- **GPS/GNSS Settings**: Multi-constellation and QZSS L1S configuration
- **NTP Server Setup**: Stratum management and client access control
- **Logging Configuration**: Local and remote Syslog setup
- **Monitoring Setup**: Prometheus metrics and health monitoring
- **Security Configuration**: Access control and DDoS protection

#### [Troubleshooting Guide](TROUBLESHOOTING.md)
Comprehensive problem diagnosis and resolution:
- **System Diagnostics**: LED indicators and web interface diagnostics
- **Power and Boot Issues**: Power supply and startup problems
- **GPS/GNSS Problems**: Satellite acquisition and signal quality issues
- **Network Connectivity**: Ethernet and DHCP troubleshooting
- **NTP Service Issues**: Server response and performance problems
- **Display Problems**: OLED display and I2C communication issues
- **Advanced Diagnostics**: Firmware recovery and professional tools
- **Maintenance Procedures**: Regular maintenance and support resources

### 🔧 Technical Documentation

#### System Requirements
- **Operating Environment**: -20°C to +70°C, <80% RH
- **Power Consumption**: ~250mA @ 3.3V typical
- **Network**: 10/100 Mbps Ethernet with PoE optional
- **Antenna**: Active GNSS antenna with >25dB gain recommended
- **Accuracy**: <1ms time accuracy with GPS+PPS synchronization

#### Performance Specifications
- **NTP Response Time**: <10ms average, <50ms maximum
- **GPS Acquisition**: <10 seconds typical, <60 seconds cold start
- **Stratum Levels**: 1 (GPS+PPS), 2 (GPS only), 3 (RTC fallback)
- **Client Capacity**: >1000 simultaneous NTP clients
- **Reliability**: >99.9% uptime in stable RF environment

### 🛠️ Development Resources

#### Software Architecture
The system follows a modular architecture with distinct subsystems:

```
Application Layer:
├── NTP Server (UDP port 123)
├── Web Interface (HTTP port 80)
├── Prometheus Metrics (/metrics endpoint)
└── OLED Display Manager

Service Layer:
├── Time Manager (GPS/PPS/RTC synchronization)
├── Network Manager (W5500 Ethernet control)
├── Configuration Manager (EEPROM persistence)
├── Logging Service (Local and Syslog)
├── System Monitor (Health and performance)
└── Error Handler (Fault detection and recovery)

Hardware Abstraction:
├── GPS Client (ZED-F9T communication)
├── Display Driver (SH1106 OLED control)
├── Network Stack (W5500 SPI interface)
└── RTC Interface (DS3231 I2C backup)
```

#### Key Protocols and Standards
- **NTP Protocol**: RFC 5905 (NTPv4) implementation
- **GNSS Standards**: GPS L1/L2, GLONASS L1/L2, Galileo E1/E5b, BeiDou B1I/B2I, QZSS L1/L2/L1S
- **Network Protocols**: Ethernet II, IP, UDP, HTTP, DHCP
- **Logging**: RFC 3164 (Syslog) with NTP facility
- **Metrics**: Prometheus exposition format
- **Time Standards**: UTC, TAI, GPS time systems

#### QZSS L1S Disaster Alert System
Special support for Japan's Quasi-Zenith Satellite System disaster alerts:
- **Signal Processing**: L1S subframe decoding from RXM-SFRBX messages
- **Alert Categories**: Earthquake, tsunami, volcanic, weather, and crisis management
- **Message Format**: JMA (Japan Meteorological Agency) standard compliance
- **Display Integration**: Real-time alert display on OLED screen
- **Logging**: Structured logging of disaster information

## Getting Started

### Quick Start Checklist

1. **Hardware Assembly** (2-4 hours):
   - Follow [Hardware Setup Guide](HARDWARE_SETUP.md)
   - Install all components and verify connections
   - Connect GNSS antenna with clear sky view
   - Apply power and verify LED startup sequence

2. **Initial Configuration** (30 minutes):
   - Connect Ethernet cable to network
   - Access web interface via DHCP-assigned IP
   - Configure network settings if needed
   - Wait for GPS satellite acquisition (5-15 minutes)

3. **System Verification** (15 minutes):
   - Verify all status LEDs are functioning correctly
   - Check GPS satellite count (minimum 4)
   - Test NTP client connectivity
   - Confirm PPS signal operation

4. **Client Configuration** (varies):
   - Configure NTP clients to use server IP address
   - Test time synchronization accuracy
   - Set up monitoring and alerting

### System Status Indicators

| LED Color | Status | Meaning |
|-----------|--------|---------|
| 🟢 Green | Solid On | GPS satellites acquired and synchronized |
| 🔵 Blue | Solid On | Ethernet network connected |
| 🟡 Yellow | 1Hz Flash | PPS signal active and synchronized |
| 🔴 Red | Off | Normal operation (no errors) |
| 🔴 Red | Solid On | System error or critical failure |

### Default Network Configuration

```yaml
Network Settings:
  Hostname: gps-ntp-server
  DHCP: Enabled (default)
  NTP Port: 123 (UDP)
  Web Interface: Port 80 (HTTP)
  Metrics Endpoint: http://[ip]/metrics

Access URLs:
  Configuration: http://[device-ip]/config
  Status Page: http://[device-ip]/
  Prometheus Metrics: http://[device-ip]/metrics
```

## Support and Community

### Documentation Updates
This documentation is maintained alongside the project codebase. For updates, corrections, or additions:
- Submit issues for documentation problems
- Contribute improvements via pull requests
- Share operational experiences and best practices

### Technical Support
- **Hardware Issues**: Check troubleshooting guide first
- **Configuration Help**: Use web interface diagnostics
- **Performance Problems**: Review monitoring metrics
- **Custom Applications**: Consult API documentation

### Community Resources
- **Project Repository**: Source code and issue tracking
- **Discussion Forums**: User community and technical discussions
- **Knowledge Base**: Collected solutions and best practices
- **Example Configurations**: Tested setups for common scenarios

## License and Disclaimers

### Usage License
This project is released under open-source license terms. See project repository for complete license information.

### Accuracy Disclaimers
- System accuracy depends on GPS signal quality and antenna installation
- Professional timing applications should include redundant time sources
- Verify accuracy requirements against system capabilities before deployment

### Safety Considerations
- Follow local RF regulations for antenna installation
- Use appropriate electrical safety practices during assembly
- Consider environmental protection for outdoor installations

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-23  
**Next Review**: 2025-04-23

For the most current information, always refer to the project repository and latest documentation releases.