#!/usr/bin/env python3
import socket
import struct
import time
import uuid
import argparse
import threading
from dataclasses import dataclass
from flask import Flask, Response

@dataclass
class PowerData:
    voltage: float = 0.0
    current: float = 0.0
    power: float = 0.0
    energy: int = 0
    frequency: float = 0.0
    power_factor: float = 0.0
    alarm_state: bool = False
    alarm_threshold: float = 0.0
    last_updated: float = 0.0

class ModbusClient:
    def __init__(self, host='192.168.1.200', port=8899, slave_addr=1, timeout=3):
        """Initialize the Modbus TCP client for RS485_TO_POE_ETH_(B)"""
        self.host = host
        self.port = port
        self.slave_addr = slave_addr
        self.timeout = timeout
        self.socket = None
        self.transaction_id = 0

    def connect(self):
        """Connect to the RS485_TO_POE_ETH device"""
        if self.socket:
            self.close()
        
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.host, self.port))
            print(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            self.socket = None
            return False

    def close(self):
        """Close the connection"""
        if self.socket:
            self.socket.close()
            self.socket = None

    def _build_modbus_frame(self, function_code, register_addr, register_count=1):
        """Build a Modbus TCP frame"""
        self.transaction_id = (self.transaction_id + 1) % 65536
        
        # Modbus TCP Header (7 bytes)
        # Transaction ID (2 bytes)
        # Protocol ID (2 bytes, always 0)
        # Length (2 bytes) - length of remaining bytes
        # Unit ID (1 byte) - slave address
        
        # For read request, the payload is 5 bytes:
        # Function code (1 byte)
        # Register address (2 bytes)
        # Register count (2 bytes)
        
        frame = struct.pack('>HHHBBH', 
                            self.transaction_id,  # Transaction ID
                            0,                   # Protocol ID (always 0)
                            6,                   # Length of remaining bytes
                            self.slave_addr,     # Unit ID
                            function_code,       # Function code
                            register_addr)       # Register address
        
        # Add register count for read operations
        if function_code in (3, 4):  # Read holding or input registers
            frame += struct.pack('>H', register_count)
        
        return frame

    def _send_receive(self, frame):
        """Send a Modbus frame and receive the response"""
        if not self.socket:
            if not self.connect():
                return None
        
        try:
            self.socket.send(frame)
            response = self.socket.recv(1024)
            return response
        except Exception as e:
            print(f"Communication error: {e}")
            self.close()
            return None

    def read_input_registers(self, start_addr, count):
        """Read input registers (function code 4)"""
        frame = self._build_modbus_frame(4, start_addr, count)
        response = self._send_receive(frame)
        
        if not response or len(response) < 9:
            print("Invalid response received")
            return None
        
        # Check for Modbus error
        function_code = response[7]
        if function_code > 0x80:
            error_code = response[8]
            print(f"Modbus error: Function code: {function_code-0x80}, Error code: {error_code}")
            return None
        
        # Extract data
        byte_count = response[8]
        if len(response) < 9 + byte_count:
            print("Response too short")
            return None
        
        # Extract register values
        registers = []
        for i in range(0, byte_count, 2):
            register = struct.unpack('>H', response[9+i:11+i])[0]
            registers.append(register)
        
        return registers

    def read_holding_registers(self, start_addr, count):
        """Read holding registers (function code 3)"""
        frame = self._build_modbus_frame(3, start_addr, count)
        response = self._send_receive(frame)
        
        if not response or len(response) < 9:
            print("Invalid response received")
            return None
        
        # Check for Modbus error
        function_code = response[7]
        if function_code > 0x80:
            error_code = response[8]
            print(f"Modbus error: Function code: {function_code-0x80}, Error code: {error_code}")
            return None
        
        # Extract data
        byte_count = response[8]
        if len(response) < 9 + byte_count:
            print("Response too short")
            return None
        
        # Extract register values
        registers = []
        for i in range(0, byte_count, 2):
            register = struct.unpack('>H', response[9+i:11+i])[0]
            registers.append(register)
        
        return registers

    def write_single_register(self, register_addr, value):
        """Write to a single holding register (function code 6)"""
        # Transaction ID, Protocol ID, Length, Unit ID, Function code, Register address, Value
        frame = struct.pack('>HHHBBHH', 
                            self.transaction_id,
                            0,                   # Protocol ID (always 0)
                            6,                   # Length of remaining bytes
                            self.slave_addr,     # Unit ID
                            6,                   # Function code
                            register_addr,       # Register address
                            value)               # Value to write
        
        response = self._send_receive(frame)
        
        if not response or len(response) < 12:
            print("Invalid response received")
            return False
        
        # Check for Modbus error
        function_code = response[7]
        if function_code > 0x80:
            error_code = response[8]
            print(f"Modbus error: Function code: {function_code-0x80}, Error code: {error_code}")
            return False
        
        return True

    def reset_energy(self):
        """Reset energy counter (custom function 0x42)"""
        # Based on the original C++ code, this is a special command
        # Transaction ID, Protocol ID, Length, Unit ID, Function code (0x42)
        frame = struct.pack('>HHHBB', 
                            self.transaction_id,
                            0,                   # Protocol ID (always 0)
                            2,                   # Length of remaining bytes
                            self.slave_addr,     # Unit ID
                            0x42)                # Custom function code for reset
        
        response = self._send_receive(frame)
        
        # For custom commands, we may not have a standard response format
        # Just check if we got any response
        if not response:
            print("No response received for reset energy command")
            return False
            
        return True

class PowerMonitor:
    def __init__(self, host='192.168.1.200', port=8899, slave_addr=1):
        """Initialize the PowerMonitor with connection to RS485_TO_POE_ETH_(B)"""
        self.modbus = ModbusClient(host, port, slave_addr)
        self.power_data = PowerData()
        self.device_id = str(uuid.uuid4())  # Generate a unique ID for this device
        
    def setup(self):
        """Setup the connection"""
        return self.modbus.connect()
        
    def request_power_data(self):
        """Request power data from the device"""
        # Read 10 registers starting from 0x0000
        registers = self.modbus.read_input_registers(0x0000, 10)
        
        if not registers or len(registers) < 10:
            print("Failed to read power data registers")
            return False
            
        # Process the register values as per the C++ code
        self.power_data.voltage = registers[0] / 10.0
        
        # Current (registers 0x0001, 0x0002)
        current_raw = registers[1] | (registers[2] << 16)
        self.power_data.current = current_raw / 1000.0
        
        # Power (registers 0x0003, 0x0004)
        power_raw = registers[3] | (registers[4] << 16)
        self.power_data.power = power_raw / 10.0
        
        # Energy (registers 0x0005, 0x0006)
        self.power_data.energy = registers[5] | (registers[6] << 16)
        
        # Frequency and Power Factor
        self.power_data.frequency = registers[7] / 10.0
        self.power_data.power_factor = registers[8] / 100.0
        
        # Alarm state
        self.power_data.alarm_state = registers[9] > 0
        
        # Update timestamp
        self.power_data.last_updated = time.time()
        
        # Get alarm threshold
        threshold = self.get_alarm_threshold()
        if threshold >= 0:
            self.power_data.alarm_threshold = threshold
        
        return True
        
    def get_alarm_threshold(self):
        """Get the current alarm threshold"""
        # Read the alarm threshold register
        registers = self.modbus.read_holding_registers(0x0001, 1)
        
        if not registers:
            print("Failed to read alarm threshold")
            return -1
            
        return registers[0]
        
    def set_alarm_threshold(self, threshold):
        """Set the alarm threshold"""
        print(f"Setting alarm threshold to {threshold}W")
        result = self.modbus.write_single_register(0x0001, int(threshold))
        
        if result:
            print("Alarm threshold set successfully")
            self.power_data.alarm_threshold = int(threshold)
        else:
            print("Failed to set alarm threshold")
            
        return result
        
    def reset_energy(self):
        """Reset the energy counter"""
        print("Resetting energy counter")
        return self.modbus.reset_energy()
        
    # Getter methods for power data
    def get_voltage(self):
        return self.power_data.voltage
        
    def get_current(self):
        return self.power_data.current
        
    def get_power(self):
        return self.power_data.power
        
    def get_energy(self):
        return self.power_data.energy
        
    def get_frequency(self):
        return self.power_data.frequency
        
    def get_power_factor(self):
        return self.power_data.power_factor
        
    def get_alarm_state(self):
        return self.power_data.alarm_state

    def print_power_data(self):
        """Print all power data to console"""
        print(f"Voltage: {self.get_voltage():.1f}V")
        print(f"Current: {self.get_current():.3f}A")
        print(f"Power: {self.get_power():.1f}W")
        print(f"Energy: {self.get_energy()}Wh")
        print(f"Frequency: {self.get_frequency():.1f}Hz")
        print(f"Power Factor: {self.get_power_factor():.2f}")
        print(f"Alarm State: {'ALARM' if self.get_alarm_state() else 'OK'}")
        print(f"Alarm Threshold: {self.power_data.alarm_threshold}W")
        
    def get_prometheus_metrics(self):
        """Generate Prometheus metrics"""
        device_label = f'device="{self.device_id}"'
        
        metrics = []
        
        # Add each metric
        metrics.append(f"# HELP ac_power_meter_voltage The AC power meter voltage. Unit 'V'.")
        metrics.append(f"# TYPE ac_power_meter_voltage gauge")
        metrics.append(f"ac_power_meter_voltage{{{device_label}}} {self.get_voltage()}")
        
        metrics.append(f"# HELP ac_power_meter_current The AC power meter current. Unit 'A'.")
        metrics.append(f"# TYPE ac_power_meter_current gauge")
        metrics.append(f"ac_power_meter_current{{{device_label}}} {self.get_current()}")
        
        metrics.append(f"# HELP ac_power_meter_power The AC power meter power. Unit 'W'.")
        metrics.append(f"# TYPE ac_power_meter_power gauge")
        metrics.append(f"ac_power_meter_power{{{device_label}}} {self.get_power()}")
        
        metrics.append(f"# HELP ac_power_meter_energy The AC power meter energy. Unit 'Wh'.")
        metrics.append(f"# TYPE ac_power_meter_energy counter")
        metrics.append(f"ac_power_meter_energy{{{device_label}}} {self.get_energy()}")
        
        metrics.append(f"# HELP ac_power_meter_frequency The AC power meter frequency. Unit 'Hz'.")
        metrics.append(f"# TYPE ac_power_meter_frequency gauge")
        metrics.append(f"ac_power_meter_frequency{{{device_label}}} {self.get_frequency()}")
        
        metrics.append(f"# HELP ac_power_meter_power_factor The AC power meter power factor. Unit 'percent'.")
        metrics.append(f"# TYPE ac_power_meter_power_factor gauge")
        metrics.append(f"ac_power_meter_power_factor{{{device_label}}} {self.get_power_factor()}")
        
        metrics.append(f"# HELP ac_power_meter_alarm_state The AC power meter alarm state. Alarmed 1 else 0.")
        metrics.append(f"# TYPE ac_power_meter_alarm_state gauge")
        metrics.append(f"ac_power_meter_alarm_state{{{device_label}}} {1 if self.get_alarm_state() else 0}")
        
        metrics.append(f"# HELP ac_power_meter_alarm_threshold The AC power meter alarm threshold. Unit 'W'.")
        metrics.append(f"# TYPE ac_power_meter_alarm_threshold gauge")
        metrics.append(f"ac_power_meter_alarm_threshold{{{device_label}}} {self.power_data.alarm_threshold}")
        
        metrics.append(f"# HELP ac_power_meter_last_updated Timestamp of last update. Unit 'seconds since epoch'.")
        metrics.append(f"# TYPE ac_power_meter_last_updated gauge")
        metrics.append(f"ac_power_meter_last_updated{{{device_label}}} {self.power_data.last_updated}")
        
        return "\n".join(metrics)

class PrometheusServer:
    def __init__(self, power_monitor, host='0.0.0.0', port=9100):
        self.power_monitor = power_monitor
        self.host = host
        self.port = port
        self.app = Flask(__name__)
        
        # Add routes
        self.app.add_url_rule('/metrics', 'metrics', self.metrics)
        self.app.add_url_rule('/', 'index', self.index)
        
    def metrics(self):
        """Prometheus metrics endpoint"""
        return Response(self.power_monitor.get_prometheus_metrics(), 
                       mimetype='text/plain')
                       
    def index(self):
        """Simple index page with links"""
        return """
        <html>
        <head><title>AC Power Monitor</title></head>
        <body>
            <h1>AC Power Monitor</h1>
            <p>This is a Prometheus exporter for AC power monitoring data.</p>
            <p><a href="/metrics">Metrics</a></p>
        </body>
        </html>
        """
        
    def run(self):
        """Run the Flask server"""
        self.app.run(host=self.host, port=self.port)
        
def monitor_thread_func(power_monitor, interval):
    """Function to run in a separate thread for monitoring"""
    try:
        while True:
            try:
                power_monitor.request_power_data()
            except Exception as e:
                print(f"Error updating power data: {e}")
            
            time.sleep(interval)
    except Exception as e:
        print(f"Monitor thread error: {e}")

def main():
    """Main function for command line usage"""
    parser = argparse.ArgumentParser(description='Power Monitor via RS485_TO_POE_ETH_(B)')
    parser.add_argument('--host', default='192.168.1.200', help='IP address of RS485_TO_POE_ETH_(B) device')
    parser.add_argument('--port', type=int, default=8899, help='Port of RS485_TO_POE_ETH_(B) device')
    parser.add_argument('--slave', type=int, default=1, help='Modbus slave address')
    parser.add_argument('--interval', type=int, default=5, help='Polling interval in seconds')
    parser.add_argument('--reset-energy', action='store_true', help='Reset energy counter')
    parser.add_argument('--set-alarm', type=int, help='Set alarm threshold (in watts)')
    parser.add_argument('--web-host', default='0.0.0.0', help='Web server host')
    parser.add_argument('--web-port', type=int, default=9100, help='Web server port')
    parser.add_argument('--no-web', action='store_true', help='Disable web server')
    
    args = parser.parse_args()
    
    # Initialize power monitor
    pm = PowerMonitor(args.host, args.port, args.slave)
    
    # Connect to device
    if not pm.setup():
        print("Failed to connect to power monitor")
        return
    
    # Handle special commands
    if args.reset_energy:
        pm.reset_energy()
    
    if args.set_alarm is not None:
        pm.set_alarm_threshold(args.set_alarm)
        
    # Print current alarm threshold
    threshold = pm.get_alarm_threshold()
    if threshold >= 0:
        print(f"Current alarm threshold: {threshold}W")
    
    if args.no_web:
        # No web server, just console monitoring
        try:
            print(f"Monitoring power data every {args.interval} seconds. Press Ctrl+C to exit.")
            while True:
                if pm.request_power_data():
                    print("\n--- Power Data ---")
                    pm.print_power_data()
                    print("-----------------")
                else:
                    print("Failed to read power data")
                
                time.sleep(args.interval)
        except KeyboardInterrupt:
            print("\nMonitoring stopped by user")
    else:
        # Start monitoring in a separate thread
        print(f"Starting power monitoring thread with {args.interval} second interval")
        monitor_thread = threading.Thread(
            target=monitor_thread_func, 
            args=(pm, args.interval),
            daemon=True
        )
        monitor_thread.start()
        
        # Start web server
        print(f"Starting Prometheus exporter on http://{args.web_host}:{args.web_port}/metrics")
        server = PrometheusServer(pm, args.web_host, args.web_port)
        try:
            server.run()
        except KeyboardInterrupt:
            print("\nServer stopped by user")
    
    # Close connection
    pm.modbus.close()

if __name__ == "__main__":
    main()