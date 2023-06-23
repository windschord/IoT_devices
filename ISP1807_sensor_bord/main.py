import time
import board
import busio
import microcontroller
import adafruit_ble_broadcastnet

import sleep_bmx055
from load_sensor import read_lsp22hb, hdc2010



print("This is BroadcastNet sensor:", adafruit_ble_broadcastnet.device_address)

with busio.I2C(board.SCL, board.SDA) as i2c:
    # sleep BMX055
    sleep_bmx055.set_sleep(i2c)

    sequence_number = 0
    while True:
        pressure = read_lsp22hb(i2c)
        humidity, temp =  hdc2010(i2c)

        measurement = adafruit_ble_broadcastnet.AdafruitSensorMeasurement()
        measurement.temperature = temp
        measurement.relative_humidity = humidity
        measurement.pressure = pressure
        measurement.sequence_number = sequence_number

        print(measurement)
        adafruit_ble_broadcastnet.broadcast(measurement=measurement, broadcast_time=1)

        sequence_number = sequence_number + 1 if sequence_number < 255 else 0 
        time.sleep(10)