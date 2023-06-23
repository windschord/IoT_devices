from adafruit_bus_device.i2c_device import I2CDevice

def set_sleep(i2c, bmx055_acc=0x18, bmx055_gyr=0x68, bmx055_mag=0x10):


    with I2CDevice(i2c, bmx055_acc) as device:
        # DEEP_SUSPEND mode
        device.write(bytes([0x11,0x20]))
    with I2CDevice(i2c, bmx055_gyr) as device:
        # DEEP_SUSPEND mode
        device.write(bytes([0x11,0x20]))
    with I2CDevice(i2c, bmx055_mag) as device:
        # Suspend mode
        device.write(bytes([0x4B,0x00]))
