import uuid
from multiprocessing import Process
from time import sleep

from adafruit_ble.advertising.standard import ManufacturerDataField
import adafruit_ble
import adafruit_ble_broadcastnet
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
from scd30_i2c import SCD30

client = InfluxDBClient(url="http://192.168.11.54:8428", org="my-org")
write_api = client.write_api(write_options=SYNCHRONOUS)
bucket = "victoriametrics"

def get_local_scd30(interval=60):
    sensor_address = ":".join([hex(n >> (5 - i) * 8 & 0xff)[2:].zfill(2) for (i, n) in enumerate([uuid.getnode()] * 6)])
    scd30 = SCD30()
    scd30.set_measurement_interval(interval)
    scd30.start_periodic_measurement()

    sleep(2)
    while True:
        if scd30.get_data_ready():
            m = scd30.read_measurement()
            if m is not None:
                print(f"CO2: {m[0]:.2f}ppm, temp: {m[1]:.2f}'C, rh: {m[2]:.2f}%")
                records = [
                    Point("home_sensors").tag("mac_addr", sensor_address).field("co2", m[0]),
                    Point("home_sensors").tag("mac_addr", sensor_address).field("temperature", m[1]),
                    Point("home_sensors").tag("mac_addr", sensor_address).field("relative_humidity", m[2]),
                ]
                write_api.write(bucket=bucket, record=records)
            sleep(interval)
        else:
            sleep(0.2)


def ble_scan():
    ble = adafruit_ble.BLERadio()

    print("scanning")
    print()

    sequence_numbers = {}

    # By providing Advertisement as well we include everything, not just specific advertisements.
    for measurement in ble.start_scan(
            adafruit_ble_broadcastnet.AdafruitSensorMeasurement, interval=0.5
    ):
        reversed_address = [measurement.address.address_bytes[i] for i in range(5, -1, -1)]
        sensor_address = "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}".format(*reversed_address)

        print(sequence_numbers)
        if sensor_address in sequence_numbers.keys() and sequence_numbers[
            sensor_address] == measurement.sequence_number:
            continue

        sequence_numbers[sensor_address] = measurement.sequence_number

        records = [
            Point("home_sensors").tag("mac_addr", sensor_address).field("sequence_number", measurement.sequence_number)
        ]

        for attribute in dir(measurement.__class__):
            attribute_instance = getattr(measurement.__class__, attribute)
            if not issubclass(attribute_instance.__class__, ManufacturerDataField):
                continue

            if attribute == "sequence_number":
                continue

            values = getattr(measurement, attribute)
            if values is None:
                continue
            else:
                records.append(
                    Point("home_sensors").tag("mac_addr", sensor_address).field(attribute, values))

        write_api.write(bucket=bucket, record=records)

        print([r.__dict__ for r in records])
        print()

    print("scan done")
    client.close()


if __name__ == "__main__":
    p1 = Process(target=get_local_scd30, args=())
    p2 = Process(target=ble_scan, args=())

    p1.start()
    p2.start()

    p1.join()
    p2.join()
