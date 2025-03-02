# AC Power Monitor - Python Implementation

This directory contains the Python implementation of the AC power monitor with Prometheus integration.

## Files

- `power_monitor.py` - Main Python script for communicating with the RS485_TO_POE_ETH_(B) device and exposing Prometheus metrics
- `test_power_monitor.py` - Unit tests with 100% code coverage
- `requirements.txt` - Python dependencies

## Usage

See the main README.md file in the parent directory for usage information.

## Quick Start

1. Install dependencies:
```bash
pip install -r requirements.txt
```

2. Start the power monitor:
```bash
python power_monitor.py --host 192.168.1.200 --port 8899
```

3. Access Prometheus metrics at:
```
http://localhost:9100/metrics
```

## Running Tests

Run the tests with:
```bash
pytest test_power_monitor.py
```

Run tests with coverage:
```bash
pytest test_power_monitor.py --cov=power_monitor --cov-report=term-missing
```

The tests include comprehensive coverage of all classes and functions in the main module.

## Configuration

The power monitor can be configured via command-line arguments. See the main README for details.