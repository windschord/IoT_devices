try:
    import struct
except ImportError:
    import ustruct as struct # pylint: disable=import-error
import time
from adafruit_bus_device.i2c_device import I2CDevice



HDC2010 = 0x40

def read_lsp22hb(i2c: I2CDevice, lsp22hb_address=0x5c):
    with I2CDevice(i2c, lsp22hb_address) as device:
        # CTRL_REG1(0x10) <- 0x50 ODR[2:0] 10Hz
        device.write(bytes([0x10,0x10]))

        # Wait Data available.
        while True:
            result = bytearray(1)
            device.write_then_readinto(bytes([0x27]),result)
            status, = struct.unpack('B',result)
            if (status & 3) == 3:
                break
            time.sleep(.005)

        # Read Pressure & Temperature
        result = bytearray(5)
        device.write_then_readinto(bytes([0x28]),result)

        # バイト列を数値に変換
        pressure_l, pressure_h, temperature = struct.unpack('<BHH',result)
        pressure = pressure_h*256 + pressure_l

        # 換算
        hPa = float(pressure) / 4096.0
        # T = float(temperature) / 100.0
        return hPa


def hdc2010(i2c: I2CDevice, hdc2010_address=0x40):
    with I2CDevice(i2c, hdc2010_address) as device:
        # 1Hz
        device.write(bytes([0x0E, 0x50]))

        # Humidity + Temperature/both 14bit/Start measurement
        device.write(bytes([0x0F, 0x01]))

        # Read
        result = bytearray(4)
        device.write_then_readinto(bytes([0x00]),result)

        # バイト列を数値に変換
        temperature, humidity = struct.unpack('<HH',result)

        # 換算
        RH = (100.0 * float(humidity)) / 65536.0
        T = (165.0 * float(temperature)) / 65536.0 - 40.0
        return RH, T