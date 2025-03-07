# AC Power Monitor

This project allows monitoring AC power parameters via RS485 Modbus devices with Prometheus integration.

## Features

- Read power measurements (voltage, current, power, energy, frequency, power factor)
- Set alarm thresholds
- Reset energy counter
- Integration with RS485_TO_POE_ETH_(B) for network connectivity
- Prometheus metrics endpoint for monitoring

## Hardware Requirements

- RS485 power monitoring device (PZEM-014 or compatible)
- Waveshare RS485_TO_POE_ETH_(B) converter
- Network infrastructure

## Usage

### Python Client

```bash
# Basic monitoring with Prometheus metrics (default port 9100)
python src_python/power_monitor.py --host 192.168.1.200 --port 8899

# Set alarm threshold
python src_python/power_monitor.py --host 192.168.1.200 --set-alarm 1000

# Reset energy counter
python src_python/power_monitor.py --host 192.168.1.200 --reset-energy

# Change polling interval
python src_python/power_monitor.py --host 192.168.1.200 --interval 5

# Change web server port (for Prometheus)
python src_python/power_monitor.py --host 192.168.1.200 --web-port 9101

# Console-only mode (no Prometheus endpoint)
python src_python/power_monitor.py --host 192.168.1.200 --no-web
```

### Command-line Arguments

#### Modbus Connection
- `--host`: IP address of the RS485_TO_POE_ETH_(B) device (default: 192.168.1.200)
- `--port`: Port number for Modbus TCP (default: 8899)
- `--slave`: Modbus slave address (default: 1)
- `--interval`: Polling interval in seconds (default: 5)
- `--reset-energy`: Reset the energy counter
- `--set-alarm`: Set the alarm threshold in watts

#### Web Server (Prometheus)
- `--web-host`: Web server host (default: 0.0.0.0)
- `--web-port`: Web server port (default: 9100)
- `--no-web`: Disable web server and only use console output

## Prometheus Integration

The application exposes a metrics endpoint at `/metrics` that is compatible with Prometheus. The following metrics are available:

- `ac_power_meter_voltage`: AC voltage in Volts
- `ac_power_meter_current`: AC current in Amperes
- `ac_power_meter_power`: Active power in Watts
- `ac_power_meter_energy`: Energy consumption in Watt-hours
- `ac_power_meter_frequency`: Line frequency in Hertz
- `ac_power_meter_power_factor`: Power factor
- `ac_power_meter_alarm_state`: Alarm status (1 if alarmed, 0 if not)
- `ac_power_meter_alarm_threshold`: Configured alarm threshold in Watts
- `ac_power_meter_last_updated`: Timestamp of last successful data update

### Prometheus Configuration Example

```yaml
scrape_configs:
  - job_name: 'ac_power_monitor'
    scrape_interval: 15s
    static_configs:
      - targets: ['localhost:9100']
```

## Configuration

Before using the Python client, ensure your RS485_TO_POE_ETH_(B) device is configured properly:

1. Configure the device to the same network as your computer
2. Set the appropriate baud rate (9600 bps recommended)
3. Set data format (8N1 - 8 data bits, no parity, 1 stop bit)
4. Configure it in TCP Server mode

## Dependencies

- Python 3.7+
- Flask (for web server)
- pytest, pytest-cov, pytest-mock (for testing)
- Standard library (socket, struct, threading, etc.)

Install dependencies using:
```bash
pip install -r src_python/requirements.txt
```

## Testing

The Python implementation includes comprehensive unit tests with 100% code coverage:

```bash
# Run tests
cd src_python
pytest test_power_monitor.py

# Run tests with coverage report
pytest test_power_monitor.py --cov=power_monitor --cov-report=term-missing
```

## ESP32 (Original) Version

The original implementation for ESP32 is still available in the `src` directory.