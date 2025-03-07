#!/usr/bin/env python3
import unittest
import socket
import struct
import time
from unittest.mock import patch, MagicMock, call
import pytest
import threading
import uuid
from flask import Flask

from power_monitor import (
    PowerData,
    ModbusClient,
    PowerMonitor,
    PrometheusServer,
    monitor_thread_func,
    main
)

class TestPowerData(unittest.TestCase):
    def test_powerdata_initialization(self):
        """Test PowerData class initialization with default values"""
        data = PowerData()
        self.assertEqual(data.voltage, 0.0)
        self.assertEqual(data.current, 0.0)
        self.assertEqual(data.power, 0.0)
        self.assertEqual(data.energy, 0)
        self.assertEqual(data.frequency, 0.0)
        self.assertEqual(data.power_factor, 0.0)
        self.assertEqual(data.alarm_state, False)
        self.assertEqual(data.alarm_threshold, 0.0)
        self.assertEqual(data.last_updated, 0.0)
        
    def test_powerdata_custom_values(self):
        """Test PowerData class initialization with custom values"""
        custom_data = PowerData(
            voltage=230.0,
            current=1.5,
            power=345.0,
            energy=1000,
            frequency=50.0,
            power_factor=0.95,
            alarm_state=True,
            alarm_threshold=500.0,
            last_updated=12345.0
        )
        self.assertEqual(custom_data.voltage, 230.0)
        self.assertEqual(custom_data.current, 1.5)
        self.assertEqual(custom_data.power, 345.0)
        self.assertEqual(custom_data.energy, 1000)
        self.assertEqual(custom_data.frequency, 50.0)
        self.assertEqual(custom_data.power_factor, 0.95)
        self.assertEqual(custom_data.alarm_state, True)
        self.assertEqual(custom_data.alarm_threshold, 500.0)
        self.assertEqual(custom_data.last_updated, 12345.0)

class TestModbusClient(unittest.TestCase):
    def setUp(self):
        """Set up ModbusClient instance for testing"""
        self.client = ModbusClient(host='192.168.1.1', port=502, slave_addr=10, timeout=2)
        
    def test_init(self):
        """Test initialization of ModbusClient"""
        self.assertEqual(self.client.host, '192.168.1.1')
        self.assertEqual(self.client.port, 502)
        self.assertEqual(self.client.slave_addr, 10)
        self.assertEqual(self.client.timeout, 2)
        self.assertIsNone(self.client.socket)
        self.assertEqual(self.client.transaction_id, 0)
        
    @patch('socket.socket')
    def test_connect_success(self, mock_socket):
        """Test successful connect"""
        mock_socket_instance = MagicMock()
        mock_socket.return_value = mock_socket_instance
        
        result = self.client.connect()
        
        mock_socket.assert_called_once_with(socket.AF_INET, socket.SOCK_STREAM)
        mock_socket_instance.settimeout.assert_called_once_with(2)
        mock_socket_instance.connect.assert_called_once_with(('192.168.1.1', 502))
        self.assertTrue(result)
        self.assertEqual(self.client.socket, mock_socket_instance)
        
    @patch('socket.socket')
    def test_connect_exception_in_socket_creation(self, mock_socket):
        """Test exception during socket creation"""
        mock_socket.side_effect = socket.error("Failed to create socket")
        
        with patch('builtins.print') as mock_print:
            result = self.client.connect()
            mock_print.assert_called_with("Connection error: Failed to create socket")
        
        self.assertFalse(result)
        self.assertIsNone(self.client.socket)
        
    @patch('socket.socket')
    def test_connect_failure(self, mock_socket):
        """Test failed connect"""
        mock_socket_instance = MagicMock()
        mock_socket.return_value = mock_socket_instance
        mock_socket_instance.connect.side_effect = Exception("Connection refused")
        
        result = self.client.connect()
        
        self.assertFalse(result)
        self.assertIsNone(self.client.socket)
        
    def test_close(self):
        """Test close socket"""
        mock_socket = MagicMock()
        self.client.socket = mock_socket
        
        self.client.close()
        
        mock_socket.close.assert_called_once()
        self.assertIsNone(self.client.socket)
        
    def test_build_modbus_frame(self):
        """Test _build_modbus_frame for various function codes"""
        # Test for function code 3 (read holding registers)
        frame = self.client._build_modbus_frame(3, 0x1000, 10)
        expected = struct.pack('>HHHBBHH', 1, 0, 6, 10, 3, 0x1000, 10)
        self.assertEqual(frame, expected)
        self.assertEqual(self.client.transaction_id, 1)
        
        # Test for function code 4 (read input registers)
        frame = self.client._build_modbus_frame(4, 0x2000, 5)
        expected = struct.pack('>HHHBBHH', 2, 0, 6, 10, 4, 0x2000, 5)
        self.assertEqual(frame, expected)
        self.assertEqual(self.client.transaction_id, 2)
        
        # Test for function code 6 (write single register)
        frame = self.client._build_modbus_frame(6, 0x3000)
        expected = struct.pack('>HHHBBH', 3, 0, 6, 10, 6, 0x3000)
        self.assertEqual(frame, expected)
        self.assertEqual(self.client.transaction_id, 3)
        
        # Test for transaction_id rollover
        self.client.transaction_id = 65535
        frame = self.client._build_modbus_frame(3, 0x4000, 1)
        expected = struct.pack('>HHHBBHH', 0, 0, 6, 10, 3, 0x4000, 1)
        self.assertEqual(frame, expected)
        self.assertEqual(self.client.transaction_id, 0)
        
    def test_send_receive_no_socket(self):
        """Test _send_receive when socket is None"""
        with patch.object(self.client, 'connect', return_value=False) as mock_connect:
            result = self.client._send_receive(b'test')
            
            mock_connect.assert_called_once()
            self.assertIsNone(result)
            
    def test_send_receive_success(self):
        """Test successful _send_receive"""
        mock_socket = MagicMock()
        mock_socket.recv.return_value = b'response'
        self.client.socket = mock_socket
        
        result = self.client._send_receive(b'test')
        
        mock_socket.send.assert_called_once_with(b'test')
        mock_socket.recv.assert_called_once_with(1024)
        self.assertEqual(result, b'response')
        
    def test_send_receive_exception(self):
        """Test _send_receive with socket error"""
        mock_socket = MagicMock()
        mock_socket.send.side_effect = Exception("Socket error")
        self.client.socket = mock_socket
        
        result = self.client._send_receive(b'test')
        
        self.assertIsNone(result)
        self.assertIsNone(self.client.socket)
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_input_registers_success(self, mock_send_receive, mock_build_frame):
        """Test successful read_input_registers"""
        # Prepare mock response (Modbus TCP header + function code + byte count + 2 registers)
        mock_response = b'\x00\x01\x00\x00\x00\x07\x0a\x04\x04\x00\x0a\x00\x14'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        result = self.client.read_input_registers(0x1000, 2)
        
        mock_build_frame.assert_called_once_with(4, 0x1000, 2)
        mock_send_receive.assert_called_once_with(b'request_frame')
        self.assertEqual(result, [10, 20])  # 0x000A = 10, 0x0014 = 20
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_input_registers_no_response(self, mock_send_receive, mock_build_frame):
        """Test read_input_registers with no response"""
        mock_send_receive.return_value = None
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_input_registers(0x1000, 2)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertIsNone(result)
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_input_registers_error_response(self, mock_send_receive, mock_build_frame):
        """Test read_input_registers with Modbus error response"""
        # Error response (function code + 0x80, error code 0x02)
        mock_response = b'\x00\x01\x00\x00\x00\x03\x0a\x84\x02'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_input_registers(0x1000, 2)
            mock_print.assert_called_with("Modbus error: Function code: 4, Error code: 2")
        
        self.assertIsNone(result)
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_input_registers_short_response(self, mock_send_receive, mock_build_frame):
        """Test read_input_registers with response that's too short"""
        mock_response = b'\x00\x01\x00\x00'  # too short
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_input_registers(0x1000, 2)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertIsNone(result)
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_input_registers_invalid_byte_count(self, mock_send_receive, mock_build_frame):
        """Test read_input_registers with invalid byte count"""
        # Byte count is 10 but actual data is shorter
        mock_response = b'\x00\x01\x00\x00\x00\x07\x0a\x04\x0a\x00\x0a\x00\x14'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_input_registers(0x1000, 2)
            mock_print.assert_called_with("Response too short")
        
        self.assertIsNone(result)
    
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_holding_registers_success(self, mock_send_receive, mock_build_frame):
        """Test successful read_holding_registers"""
        # Prepare mock response (Modbus TCP header + function code + byte count + 2 registers)
        mock_response = b'\x00\x01\x00\x00\x00\x07\x0a\x03\x04\x00\x0a\x00\x14'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        result = self.client.read_holding_registers(0x1000, 2)
        
        mock_build_frame.assert_called_once_with(3, 0x1000, 2)
        mock_send_receive.assert_called_once_with(b'request_frame')
        self.assertEqual(result, [10, 20])  # 0x000A = 10, 0x0014 = 20
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_holding_registers_error_response(self, mock_send_receive, mock_build_frame):
        """Test read_holding_registers with Modbus error response"""
        # Error response
        mock_response = b'\x00\x01\x00\x00\x00\x03\x0a\x83\x02'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_holding_registers(0x1000, 2)
            mock_print.assert_called_with("Modbus error: Function code: 3, Error code: 2")
        
        self.assertIsNone(result)
        
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_read_holding_registers_short_response(self, mock_send_receive, mock_build_frame):
        """Test read_holding_registers with short response"""
        mock_response = b'\x00\x01\x00\x00'
        mock_send_receive.return_value = mock_response
        mock_build_frame.return_value = b'request_frame'
        
        with patch('builtins.print') as mock_print:
            result = self.client.read_holding_registers(0x1000, 2)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertIsNone(result)
    
    @patch.object(ModbusClient, '_build_modbus_frame')
    @patch.object(ModbusClient, '_send_receive')
    def test_write_single_register_success(self, mock_send_receive, mock_build_frame):
        """Test successful write_single_register"""
        # Prepare mock response (success echo)
        mock_response = b'\x00\x01\x00\x00\x00\x06\x0a\x06\x10\x00\x00\x0a'
        mock_send_receive.return_value = mock_response
        
        frame = struct.pack('>HHHBBHH', 1, 0, 6, 10, 6, 0x1000, 10)
        self.client.transaction_id = 1
        
        result = self.client.write_single_register(0x1000, 10)
        
        mock_send_receive.assert_called_once()
        self.assertTrue(result)
    
    @patch.object(ModbusClient, '_send_receive')
    def test_write_single_register_error_short(self, mock_send_receive):
        """Test write_single_register with short response"""
        # Mock response that's too short to trigger invalid response
        mock_response = b'\x00\x01'
        mock_send_receive.return_value = mock_response
        
        with patch('builtins.print') as mock_print:
            result = self.client.write_single_register(0x1000, 10)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertFalse(result)
        
    @patch.object(ModbusClient, '_send_receive')
    def test_write_single_register_error_modbus(self, mock_send_receive):
        """Test write_single_register with Modbus error response"""
        # Error response with function code + 0x80, need at least 12 bytes of response
        mock_response = b'\x00\x01\x00\x00\x00\x06\x0a\x86\x02\x00\x00\x00'
        mock_send_receive.return_value = mock_response
        
        with patch('builtins.print') as mock_print:
            result = self.client.write_single_register(0x1000, 10)
            mock_print.assert_called_with("Modbus error: Function code: 6, Error code: 2")
        
        self.assertFalse(result)
    
    @patch.object(ModbusClient, '_send_receive')
    def test_write_single_register_no_response(self, mock_send_receive):
        """Test write_single_register with no response"""
        mock_send_receive.return_value = None
        
        with patch('builtins.print') as mock_print:
            result = self.client.write_single_register(0x1000, 10)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertFalse(result)
    
    @patch.object(ModbusClient, '_send_receive')
    def test_write_single_register_medium_response(self, mock_send_receive):
        """Test write_single_register with medium length response (not short enough for invalid but shorter than required)"""
        mock_response = b'\x00\x01\x00\x00\x00\x06\x0a\x06\x10\x00'  # 10 bytes, need 12
        mock_send_receive.return_value = mock_response
        
        with patch('builtins.print') as mock_print:
            result = self.client.write_single_register(0x1000, 10)
            mock_print.assert_called_with("Invalid response received")
        
        self.assertFalse(result)
    
    @patch.object(ModbusClient, '_send_receive')
    def test_reset_energy_success(self, mock_send_receive):
        """Test successful reset_energy"""
        mock_response = b'response'
        mock_send_receive.return_value = mock_response
        
        result = self.client.reset_energy()
        
        self.assertTrue(result)
    
    @patch.object(ModbusClient, '_send_receive')
    def test_reset_energy_no_response(self, mock_send_receive):
        """Test reset_energy with no response"""
        mock_send_receive.return_value = None
        
        with patch('builtins.print') as mock_print:
            result = self.client.reset_energy()
            mock_print.assert_called_with("No response received for reset energy command")
        
        self.assertFalse(result)

class TestPowerMonitor(unittest.TestCase):
    def setUp(self):
        """Set up PowerMonitor instance for testing"""
        # Patch uuid.uuid4 to return a consistent value
        patcher = patch('uuid.uuid4', return_value=uuid.UUID('12345678-1234-5678-1234-567812345678'))
        self.addCleanup(patcher.stop)
        patcher.start()
        
        # Create PowerMonitor with mock ModbusClient
        self.mock_modbus = MagicMock()
        with patch('power_monitor.ModbusClient', return_value=self.mock_modbus):
            self.power_monitor = PowerMonitor(host='192.168.1.1', port=502, slave_addr=10)
    
    def test_init(self):
        """Test PowerMonitor initialization"""
        self.assertEqual(self.power_monitor.device_id, '12345678-1234-5678-1234-567812345678')
        self.assertIsInstance(self.power_monitor.power_data, PowerData)
        
    def test_setup(self):
        """Test setup method"""
        self.mock_modbus.connect.return_value = True
        result = self.power_monitor.setup()
        
        self.mock_modbus.connect.assert_called_once()
        self.assertTrue(result)
        
    def test_request_power_data_success(self):
        """Test successful request_power_data"""
        # Prepare mock register values
        registers = [240, 100, 0, 2500, 0, 1000, 0, 500, 95, 1]  # voltage, current, power, energy, freq, pf, alarm
        self.mock_modbus.read_input_registers.return_value = registers
        self.mock_modbus.read_holding_registers.return_value = [1000]  # alarm threshold
        
        with patch('time.time', return_value=12345.0):
            result = self.power_monitor.request_power_data()
            
        self.mock_modbus.read_input_registers.assert_called_once_with(0x0000, 10)
        self.mock_modbus.read_holding_registers.assert_called_once_with(0x0001, 1)
        self.assertTrue(result)
        
        # Check PowerData values
        self.assertEqual(self.power_monitor.power_data.voltage, 24.0)  # 240/10
        self.assertEqual(self.power_monitor.power_data.current, 0.1)   # 100/1000
        self.assertEqual(self.power_monitor.power_data.power, 250.0)   # 2500/10
        self.assertEqual(self.power_monitor.power_data.energy, 1000)   # 1000
        self.assertEqual(self.power_monitor.power_data.frequency, 50.0) # 500/10
        self.assertEqual(self.power_monitor.power_data.power_factor, 0.95) # 95/100
        self.assertTrue(self.power_monitor.power_data.alarm_state)  # 1
        self.assertEqual(self.power_monitor.power_data.alarm_threshold, 1000)
        self.assertEqual(self.power_monitor.power_data.last_updated, 12345.0)
        
    def test_request_power_data_failure(self):
        """Test request_power_data failure"""
        self.mock_modbus.read_input_registers.return_value = None
        
        with patch('builtins.print') as mock_print:
            result = self.power_monitor.request_power_data()
            mock_print.assert_called_with("Failed to read power data registers")
        
        self.assertFalse(result)
        
    def test_request_power_data_short_registers(self):
        """Test request_power_data with short register list"""
        self.mock_modbus.read_input_registers.return_value = [1, 2, 3]  # too short
        
        with patch('builtins.print') as mock_print:
            result = self.power_monitor.request_power_data()
            mock_print.assert_called_with("Failed to read power data registers")
        
        self.assertFalse(result)
        
    def test_get_alarm_threshold_success(self):
        """Test get_alarm_threshold success"""
        self.mock_modbus.read_holding_registers.return_value = [1500]
        
        result = self.power_monitor.get_alarm_threshold()
        
        self.mock_modbus.read_holding_registers.assert_called_once_with(0x0001, 1)
        self.assertEqual(result, 1500)
        
    def test_get_alarm_threshold_failure(self):
        """Test get_alarm_threshold failure"""
        self.mock_modbus.read_holding_registers.return_value = None
        
        result = self.power_monitor.get_alarm_threshold()
        
        self.assertEqual(result, -1)
        
    def test_set_alarm_threshold_success(self):
        """Test set_alarm_threshold success"""
        self.mock_modbus.write_single_register.return_value = True
        
        result = self.power_monitor.set_alarm_threshold(2000)
        
        self.mock_modbus.write_single_register.assert_called_once_with(0x0001, 2000)
        self.assertTrue(result)
        self.assertEqual(self.power_monitor.power_data.alarm_threshold, 2000)
        
    def test_set_alarm_threshold_failure(self):
        """Test set_alarm_threshold failure"""
        self.mock_modbus.write_single_register.return_value = False
        
        result = self.power_monitor.set_alarm_threshold(2000)
        
        self.assertFalse(result)
        
    def test_reset_energy(self):
        """Test reset_energy"""
        self.mock_modbus.reset_energy.return_value = True
        
        result = self.power_monitor.reset_energy()
        
        self.mock_modbus.reset_energy.assert_called_once()
        self.assertTrue(result)
        
    def test_getter_methods(self):
        """Test all getter methods"""
        # Set up test data
        self.power_monitor.power_data = PowerData(
            voltage=230.0,
            current=1.5,
            power=345.0,
            energy=1000,
            frequency=50.0,
            power_factor=0.95,
            alarm_state=True
        )
        
        # Test each getter
        self.assertEqual(self.power_monitor.get_voltage(), 230.0)
        self.assertEqual(self.power_monitor.get_current(), 1.5)
        self.assertEqual(self.power_monitor.get_power(), 345.0)
        self.assertEqual(self.power_monitor.get_energy(), 1000)
        self.assertEqual(self.power_monitor.get_frequency(), 50.0)
        self.assertEqual(self.power_monitor.get_power_factor(), 0.95)
        self.assertEqual(self.power_monitor.get_alarm_state(), True)
        
    def test_print_power_data(self):
        """Test print_power_data"""
        # Set up test data
        self.power_monitor.power_data = PowerData(
            voltage=230.0,
            current=1.5,
            power=345.0,
            energy=1000,
            frequency=50.0,
            power_factor=0.95,
            alarm_state=True,
            alarm_threshold=500.0
        )
        
        with patch('builtins.print') as mock_print:
            self.power_monitor.print_power_data()
            
            # Check that print was called 8 times (once for each metric)
            self.assertEqual(mock_print.call_count, 8)
            
    def test_get_prometheus_metrics(self):
        """Test get_prometheus_metrics"""
        # Set up test data
        self.power_monitor.power_data = PowerData(
            voltage=230.0,
            current=1.5,
            power=345.0,
            energy=1000,
            frequency=50.0,
            power_factor=0.95,
            alarm_state=True,
            alarm_threshold=500.0,
            last_updated=12345.6
        )
        
        metrics = self.power_monitor.get_prometheus_metrics()
        
        # Check that metrics contains expected format and values
        self.assertIn('ac_power_meter_voltage{device="12345678-1234-5678-1234-567812345678"} 230.0', metrics)
        self.assertIn('ac_power_meter_current{device="12345678-1234-5678-1234-567812345678"} 1.5', metrics)
        self.assertIn('ac_power_meter_power{device="12345678-1234-5678-1234-567812345678"} 345.0', metrics)
        self.assertIn('ac_power_meter_energy{device="12345678-1234-5678-1234-567812345678"} 1000', metrics)
        self.assertIn('ac_power_meter_frequency{device="12345678-1234-5678-1234-567812345678"} 50.0', metrics)
        self.assertIn('ac_power_meter_power_factor{device="12345678-1234-5678-1234-567812345678"} 0.95', metrics)
        self.assertIn('ac_power_meter_alarm_state{device="12345678-1234-5678-1234-567812345678"} 1', metrics)
        self.assertIn('ac_power_meter_alarm_threshold{device="12345678-1234-5678-1234-567812345678"} 500.0', metrics)
        self.assertIn('ac_power_meter_last_updated{device="12345678-1234-5678-1234-567812345678"} 12345.6', metrics)

class TestPrometheusServer(unittest.TestCase):
    def setUp(self):
        """Set up PrometheusServer for testing"""
        self.mock_power_monitor = MagicMock()
        self.mock_power_monitor.get_prometheus_metrics.return_value = "metrics data"
        
        self.server = PrometheusServer(self.mock_power_monitor, host='localhost', port=8000)
        
    def test_init(self):
        """Test PrometheusServer initialization"""
        self.assertEqual(self.server.power_monitor, self.mock_power_monitor)
        self.assertEqual(self.server.host, 'localhost')
        self.assertEqual(self.server.port, 8000)
        self.assertIsInstance(self.server.app, Flask)
        
    def test_metrics(self):
        """Test metrics endpoint"""
        with self.server.app.test_client() as client:
            response = client.get('/metrics')
            
            self.mock_power_monitor.get_prometheus_metrics.assert_called_once()
            self.assertEqual(response.status_code, 200)
            self.assertEqual(response.data.decode(), "metrics data")
            self.assertEqual(response.mimetype, 'text/plain')
            
    def test_index(self):
        """Test index endpoint"""
        with self.server.app.test_client() as client:
            response = client.get('/')
            
            self.assertEqual(response.status_code, 200)
            self.assertIn(b'<h1>AC Power Monitor</h1>', response.data)
            self.assertIn(b'<a href="/metrics">Metrics</a>', response.data)
            
    @patch('flask.Flask.run')
    def test_run(self, mock_run):
        """Test run method"""
        self.server.run()
        
        mock_run.assert_called_once_with(host='localhost', port=8000)

class TestMonitorThreadFunc(unittest.TestCase):
    def test_monitor_thread_func(self):
        """Test monitor_thread_func"""
        mock_power_monitor = MagicMock()
        
        # Mock time.sleep to avoid actually sleeping and to exit after a few iterations
        with patch('time.sleep', side_effect=[None, None, Exception("Stop")]) as mock_sleep:
            try:
                monitor_thread_func(mock_power_monitor, 5)
            except Exception as e:
                self.assertEqual(str(e), "Stop")
                
            # Check that request_power_data was called for each iteration
            self.assertEqual(mock_power_monitor.request_power_data.call_count, 3)
            # Check that sleep was called with the correct interval
            mock_sleep.assert_has_calls([call(5), call(5), call(5)])
            
    def test_monitor_thread_func_with_thread_exception(self):
        """Test monitor_thread_func with a thread-level exception"""
        mock_power_monitor = MagicMock()
        
        # First, let's try with a thread-level exception outside of request_power_data
        with patch('time.sleep') as mock_sleep:
            mock_sleep.side_effect = Exception("Thread crashed")
            
            with patch('builtins.print') as mock_print:
                try:
                    monitor_thread_func(mock_power_monitor, 5)
                except Exception:
                    # Exception should be caught inside the function
                    pass
                
                mock_print.assert_called_with("Monitor thread error: Thread crashed")
            
    def test_monitor_thread_func_request_exception(self):
        """Test monitor_thread_func handling exception from request_power_data"""
        mock_power_monitor = MagicMock()
        mock_power_monitor.request_power_data.side_effect = [Exception("Request error"), None, Exception("Stop")]
        
        # Mock time.sleep to avoid actually sleeping and to exit after a few iterations
        with patch('time.sleep', side_effect=[None, None, Exception("Stop")]) as mock_sleep:
            try:
                monitor_thread_func(mock_power_monitor, 5)
            except Exception as e:
                self.assertEqual(str(e), "Stop")
                
            # Check that the function continued despite the request error
            self.assertEqual(mock_power_monitor.request_power_data.call_count, 3)

class TestMain(unittest.TestCase):
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    @patch('power_monitor.PrometheusServer')
    @patch('threading.Thread')
    def test_main_with_web_server(self, mock_thread, mock_server, mock_power_monitor, mock_parse_args):
        """Test main function with web server"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        mock_args.interval = 5
        mock_args.reset_energy = False
        mock_args.set_alarm = None
        mock_args.web_host = 'localhost'
        mock_args.web_port = 8000
        mock_args.no_web = False
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = True
        mock_pm_instance.get_alarm_threshold.return_value = 1000
        
        mock_server_instance = MagicMock()
        mock_server.return_value = mock_server_instance
        mock_server_instance.run.side_effect = KeyboardInterrupt()
        
        mock_thread_instance = MagicMock()
        mock_thread.return_value = mock_thread_instance
        
        # Run main
        with patch('builtins.print') as mock_print:
            main()
        
        # Check PowerMonitor initialization and methods
        mock_power_monitor.assert_called_once_with('192.168.1.1', 502, 10)
        mock_pm_instance.setup.assert_called_once()
        mock_pm_instance.get_alarm_threshold.assert_called_once()
        
        # Check thread was created and started
        mock_thread.assert_called_once()
        mock_thread_instance.start.assert_called_once()
        
        # Check PrometheusServer was created and run
        mock_server.assert_called_once_with(mock_pm_instance, 'localhost', 8000)
        mock_server_instance.run.assert_called_once()
        
        # Check connection was closed
        mock_pm_instance.modbus.close.assert_called_once()
        
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    @patch('power_monitor.PrometheusServer')
    @patch('threading.Thread')
    def test_main_web_server_keyboard_interrupt(self, mock_thread, mock_server, mock_power_monitor, mock_parse_args):
        """Test main function with web server handling KeyboardInterrupt properly"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        mock_args.interval = 5
        mock_args.reset_energy = False
        mock_args.set_alarm = None
        mock_args.web_host = 'localhost'
        mock_args.web_port = 8000
        mock_args.no_web = False
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = True
        mock_pm_instance.get_alarm_threshold.return_value = 1000
        
        mock_server_instance = MagicMock()
        mock_server.return_value = mock_server_instance
        # This simulates the user pressing Ctrl+C
        mock_server_instance.run.side_effect = KeyboardInterrupt()
        
        mock_thread_instance = MagicMock()
        mock_thread.return_value = mock_thread_instance
        
        # Run main - KeyboardInterrupt should be handled gracefully
        with patch('builtins.print') as mock_print:
            main()
            mock_print.assert_any_call("\nServer stopped by user")
            
        # Check resources were cleaned up
        mock_pm_instance.modbus.close.assert_called_once()
        
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    @patch('builtins.print')
    def test_main_setup_failure(self, mock_print, mock_power_monitor, mock_parse_args):
        """Test main function with setup failure"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = False
        
        # Run main
        main()
        
        # Check early return
        mock_pm_instance.get_alarm_threshold.assert_not_called()
        
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    def test_main_with_reset_energy(self, mock_power_monitor, mock_parse_args):
        """Test main function with reset_energy"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        mock_args.reset_energy = True
        mock_args.set_alarm = None
        mock_args.no_web = True  # To avoid web server setup
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = True
        mock_pm_instance.get_alarm_threshold.return_value = 1000
        
        # Need to break out of the monitoring loop
        mock_pm_instance.request_power_data.side_effect = KeyboardInterrupt()
        
        # Run main
        with patch('builtins.print'):
            main()
        
        # Check reset_energy was called
        mock_pm_instance.reset_energy.assert_called_once()
        
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    def test_main_with_set_alarm(self, mock_power_monitor, mock_parse_args):
        """Test main function with set_alarm"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        mock_args.reset_energy = False
        mock_args.set_alarm = 1500
        mock_args.no_web = True  # To avoid web server setup
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = True
        mock_pm_instance.get_alarm_threshold.return_value = 1000
        
        # Need to break out of the monitoring loop
        mock_pm_instance.request_power_data.side_effect = KeyboardInterrupt()
        
        # Run main
        with patch('builtins.print'):
            main()
        
        # Check set_alarm_threshold was called
        mock_pm_instance.set_alarm_threshold.assert_called_once_with(1500)
        
    @patch('argparse.ArgumentParser.parse_args')
    @patch('power_monitor.PowerMonitor')
    @patch('time.sleep')
    def test_main_with_console_mode(self, mock_sleep, mock_power_monitor, mock_parse_args):
        """Test main function with console mode"""
        # Set up mocks
        mock_args = MagicMock()
        mock_args.host = '192.168.1.1'
        mock_args.port = 502
        mock_args.slave = 10
        mock_args.interval = 5
        mock_args.reset_energy = False
        mock_args.set_alarm = None
        mock_args.no_web = True
        
        mock_parse_args.return_value = mock_args
        
        mock_pm_instance = MagicMock()
        mock_power_monitor.return_value = mock_pm_instance
        mock_pm_instance.setup.return_value = True
        mock_pm_instance.get_alarm_threshold.return_value = 1000
        
        # Set up request_power_data to test both success and failure paths
        mock_pm_instance.request_power_data.side_effect = [True, False, KeyboardInterrupt()]
        
        # Run main
        with patch('builtins.print') as mock_print:
            main()
            mock_print.assert_any_call("Failed to read power data")
        
        # Check request_power_data and print_power_data were called
        self.assertEqual(mock_pm_instance.request_power_data.call_count, 3)
        mock_pm_instance.print_power_data.assert_called_once()
        
        # Check sleep was called with correct interval twice
        mock_sleep.assert_has_calls([call(5), call(5)])

class TestModuleExecution(unittest.TestCase):
    def test_main_function(self):
        """Test that the main function is called when run as script"""
        # Just check if the if __name__ == "__main__" line is in the file
        with open('power_monitor.py', 'r') as f:
            content = f.read()
            self.assertIn('if __name__ == "__main__":', content)
            self.assertIn('main()', content)
            
    @patch('power_monitor.main')
    def test_module_execution(self, mock_main):
        """Test module execution when run directly"""
        # Create a proper module-like object with __name__ == "__main__"
        module_globals = {'__name__': '__main__', 'main': mock_main}
        # Extract the if __name__ == "__main__" code
        with open('power_monitor.py', 'r') as f:
            content = f.read()
            main_check = content.split('if __name__ == "__main__":')[1].strip()
            # Execute it in our custom namespace
            exec(main_check, module_globals)
            # Verify main was called
            mock_main.assert_called_once()

if __name__ == '__main__':
    unittest.main()