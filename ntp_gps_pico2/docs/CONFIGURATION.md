# GPS NTP Server - Configuration Guide

## Overview

This guide covers the configuration and operation procedures for the GPS NTP Server system. After completing the hardware setup, follow these instructions to configure the software and establish proper system operation.

## Initial System Configuration

### First Boot Procedure

1. **Power-On Sequence**:
   - Connect power to the system
   - Observe LED status indicators during boot:
     - All LEDs flash briefly (system initialization)
     - Red LED remains on until system ready
     - Blue LED turns on when network connects
     - Green LED turns on when GPS acquires satellites
     - Yellow LED flashes at 1Hz when PPS is active

2. **Network Connection**:
   - Connect Ethernet cable to your network
   - System will attempt DHCP by default
   - Note the assigned IP address from your router's DHCP table

3. **Initial GPS Acquisition**:
   - GPS acquisition may take 5-15 minutes for first fix
   - Green LED will turn on when satellites are acquired
   - Yellow LED starts flashing when PPS signal is active

### Web Interface Access

The system provides a web interface for configuration and monitoring:

1. **Access URL**: `http://[device-ip-address]/`
2. **Configuration Page**: `http://[device-ip-address]/config`
3. **Metrics Endpoint**: `http://[device-ip-address]/metrics`

### Default Configuration

```json
{
  "network": {
    "hostname": "gps-ntp-server",
    "ip_address": 0,
    "netmask": 0,
    "gateway": 0,
    "dns_server": 0,
    "dhcp_enabled": true
  },
  "ntp": {
    "enabled": true,
    "port": 123,
    "stratum": 1,
    "precision": -20
  },
  "gps": {
    "enabled": true,
    "constellations": {
      "gps": true,
      "glonass": true,
      "galileo": true,
      "beidou": true,
      "qzss": true
    },
    "qzss_l1s_enabled": true,
    "update_rate": 1
  },
  "logging": {
    "level": "INFO",
    "syslog_server": "",
    "syslog_port": 514,
    "local_buffering": true,
    "max_buffer_entries": 50
  },
  "monitoring": {
    "prometheus_enabled": true,
    "prometheus_port": 80,
    "metrics_update_interval": 30
  },
  "display": {
    "enabled": true,
    "brightness": 128,
    "auto_mode_switch": true,
    "mode_switch_interval": 10
  }
}
```

## Network Configuration

### DHCP Configuration (Default)

The system uses DHCP by default. No configuration required if your network supports DHCP.

### Static IP Configuration

To configure a static IP address:

1. **Via Web Interface**:
   - Navigate to `http://[device-ip]/config`
   - Set "DHCP Enabled" to false
   - Enter desired IP address, netmask, gateway, and DNS server
   - Click "Save Configuration"

2. **Via Configuration File**:
   ```json
   {
     "network": {
       "dhcp_enabled": false,
       "ip_address": "192.168.1.100",
       "netmask": "255.255.255.0",
       "gateway": "192.168.1.1",
       "dns_server": "8.8.8.8"
     }
   }
   ```

### Network Troubleshooting

**No Network Connection (Blue LED Off)**:
- Check Ethernet cable connection
- Verify cable integrity
- Check switch/router port status
- Verify power to network equipment

**DHCP Issues**:
- Check DHCP server availability
- Verify DHCP scope has available addresses
- Check MAC address filtering (if enabled)
- Consider static IP configuration

## GPS/GNSS Configuration

### Multi-Constellation Setup

The system supports multiple GNSS constellations:

```json
{
  "gps": {
    "constellations": {
      "gps": true,        // GPS (USA)
      "glonass": true,    // GLONASS (Russia)
      "galileo": true,    // Galileo (EU)
      "beidou": true,     // BeiDou (China)
      "qzss": true        // QZSS (Japan)
    }
  }
}
```

**Recommended Settings**:
- **Urban Areas**: Enable GPS + Galileo + QZSS
- **Rural Areas**: Enable all constellations for maximum coverage
- **Japan**: Always enable QZSS for L1S disaster alerts

### QZSS L1S Disaster Alert Configuration

QZSS L1S provides disaster and crisis management alerts for Japan:

```json
{
  "gps": {
    "qzss_l1s_enabled": true,
    "disaster_alert_priority": 1
  }
}
```

**Alert Categories**:
- Earthquake warnings
- Tsunami warnings  
- Volcanic activity alerts
- Weather emergencies
- Other crisis management information

### GPS Performance Optimization

**Antenna Placement**:
- Clear sky view (minimal obstructions)
- Away from RF interference sources
- Stable mounting to prevent vibration

**Configuration Parameters**:
```json
{
  "gps": {
    "update_rate": 1,           // 1Hz update rate
    "elevation_mask": 5,        // Minimum satellite elevation (degrees)
    "signal_threshold": 25,     // Minimum C/N0 ratio (dB-Hz)
    "position_hold": true       // Enable position hold mode
  }
}
```

## NTP Server Configuration

### Basic NTP Settings

```json
{
  "ntp": {
    "enabled": true,
    "port": 123,
    "stratum": 1,
    "precision": -20,
    "root_delay": 0,
    "root_dispersion": 10,
    "reference_id": "GPS"
  }
}
```

### Client Access Control

Configure NTP client access restrictions:

```json
{
  "ntp": {
    "access_control": {
      "enabled": true,
      "allow_networks": [
        "192.168.1.0/24",
        "10.0.0.0/8"
      ],
      "deny_networks": [
        "0.0.0.0/0"
      ],
      "rate_limit": {
        "enabled": true,
        "requests_per_second": 10,
        "burst_limit": 50
      }
    }
  }
}
```

### Stratum Configuration

The system automatically manages stratum levels:

- **Stratum 1**: GPS synchronized with PPS active
- **Stratum 2**: GPS synchronized but no PPS
- **Stratum 3**: GPS signal lost, using RTC backup
- **Stratum 16**: System initialization or critical failure

## Logging Configuration

### Local Logging

```json
{
  "logging": {
    "level": "INFO",
    "local_buffering": true,
    "max_buffer_entries": 50,
    "rotation": {
      "enabled": true,
      "max_size": 1024,
      "max_files": 5
    }
  }
}
```

**Log Levels** (from highest to lowest priority):
- `EMERGENCY`: System unusable
- `ALERT`: Action must be taken immediately  
- `CRITICAL`: Critical conditions
- `ERROR`: Error conditions
- `WARNING`: Warning conditions
- `NOTICE`: Normal but significant conditions
- `INFO`: Informational messages
- `DEBUG`: Debug-level messages

### Syslog Configuration

To send logs to a remote syslog server:

```json
{
  "logging": {
    "syslog_server": "192.168.1.10",
    "syslog_port": 514,
    "facility": "NTP",
    "protocol": "UDP"
  }
}
```

## Monitoring and Metrics

### Prometheus Metrics

The system exposes metrics in Prometheus format at `/metrics`:

**Key Metrics**:
- `ntp_requests_total`: Total NTP requests received
- `ntp_responses_total`: Total NTP responses sent
- `ntp_response_time_ms`: Average response time
- `gps_satellites_total`: Number of satellites in view
- `gps_hdop`: Horizontal dilution of precision
- `system_uptime_seconds`: System uptime
- `system_ram_usage_percent`: RAM usage percentage

### System Health Monitoring

Health score calculation (0-100%):

```
Base Score: 100%
Deductions:
- RAM usage > 80%: -1% per percent over 80%
- GPS satellites < 4: -5% per missing satellite
- Network disconnected: -15%
- GPS fallback mode: -20%
- CPU temperature > 45°C: -2% per degree over 45°C
```

### Display Configuration

The OLED display cycles through 5 modes:

1. **GPS Time Display**: Current time and date
2. **Satellite Status**: Constellation-specific satellite counts
3. **NTP Statistics**: Request/response statistics
4. **System Status**: Uptime, memory, temperature
5. **Error Messages**: Active alerts and warnings

```json
{
  "display": {
    "mode_switch_interval": 10,
    "brightness": 128,
    "auto_mode_switch": true,
    "show_qzss_alerts": true
  }
}
```

## Security Configuration

### Network Security

**Access Control Lists**:
```json
{
  "security": {
    "ntp": {
      "allow_networks": ["192.168.0.0/16", "10.0.0.0/8"],
      "deny_all_others": true,
      "rate_limiting": {
        "requests_per_minute": 60,
        "burst_allowance": 10
      }
    }
  }
}
```

**DDoS Protection**:
- Rate limiting: 60 requests/minute per client
- Burst allowance: 10 requests
- Automatic blacklisting for abuse

### Configuration Security

**Web Interface Protection**:
- Configuration changes require confirmation
- Invalid settings are rejected with error messages
- Configuration rollback on critical failures

## Operational Procedures

### Daily Operations

**Morning Checklist**:
1. Check LED status indicators:
   - Green (GPS): Should be solid on
   - Blue (Network): Should be solid on  
   - Yellow (PPS): Should flash at 1Hz
   - Red (Error): Should be off
2. Verify system time accuracy
3. Check NTP client connectivity
4. Review any overnight alerts

**System Monitoring**:
- Monitor GPS satellite count (minimum 4)
- Check network connectivity
- Verify NTP response times (<10ms)
- Monitor system health score (>80%)

### Weekly Maintenance

1. **Performance Review**:
   - Analyze NTP request patterns
   - Review GPS signal quality trends
   - Check system resource usage

2. **Configuration Backup**:
   - Export current configuration
   - Document any changes made
   - Verify backup procedures

3. **System Updates**:
   - Check for firmware updates
   - Review security patches
   - Plan maintenance windows

### Monthly Tasks

1. **Comprehensive Testing**:
   - Test GPS failover scenarios
   - Verify network redundancy
   - Check backup power systems

2. **Performance Optimization**:
   - Analyze long-term trends
   - Optimize constellation settings
   - Review antenna placement

3. **Documentation Updates**:
   - Update configuration records
   - Document operational changes
   - Review troubleshooting procedures

## Configuration Examples

### Corporate Network Setup

```json
{
  "network": {
    "hostname": "corp-ntp-01",
    "dhcp_enabled": false,
    "ip_address": "10.1.100.50",
    "netmask": "255.255.255.0",
    "gateway": "10.1.100.1",
    "dns_server": "10.1.1.10"
  },
  "ntp": {
    "access_control": {
      "allow_networks": ["10.1.0.0/16"],
      "rate_limit": {
        "requests_per_second": 20
      }
    }
  },
  "logging": {
    "syslog_server": "10.1.1.20",
    "level": "NOTICE"
  }
}
```

### Home Network Setup

```json
{
  "network": {
    "hostname": "home-ntp",
    "dhcp_enabled": true
  },
  "ntp": {
    "access_control": {
      "allow_networks": ["192.168.1.0/24"]
    }
  },
  "gps": {
    "constellations": {
      "gps": true,
      "galileo": true,
      "qzss": false
    }
  }
}
```

### Research/Scientific Setup

```json
{
  "gps": {
    "constellations": {
      "gps": true,
      "glonass": true,
      "galileo": true,
      "beidou": true,
      "qzss": true
    },
    "update_rate": 1,
    "position_hold": true
  },
  "ntp": {
    "stratum": 1,
    "precision": -21
  },
  "logging": {
    "level": "DEBUG",
    "local_buffering": true
  }
}
```

## Next Steps

After completing the configuration:

1. **System Testing**: Perform comprehensive functionality tests
2. **Client Configuration**: Configure NTP clients to use your server
3. **Monitoring Setup**: Integrate with your monitoring systems
4. **Documentation**: Record your specific configuration settings

Proceed to the [Troubleshooting Guide](TROUBLESHOOTING.md) for problem resolution procedures.