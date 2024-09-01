from time import sleep
import network
from webserver import load_data

AP_SSID = 'RPW_WIFI'
AP_PASSWORD = '123456789'
AP_RETRY_COUNT = 30

STORE_FILENAME = 'save_data.json'


def connect_ap(ssid: str, password: str):
    '''
    Connect to the Wi-Fi network

    '''
    print('Connecting to network...')
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)

    count = 0
    while wlan.isconnected() == False and count < AP_RETRY_COUNT:
        print(f'Waiting for connection... {count}')
        count += 1
        sleep(1)

    if count >= AP_RETRY_COUNT:
        print('Connection failed. fallback to AP mode')
        wlan = start_ap()
    return wlan


def start_ap():
    '''
    Start the Access Point
    '''
    print('Starting AP...')
    wlan = network.WLAN(network.AP_IF)
    wlan.config(essid=AP_SSID, password=AP_PASSWORD)
    wlan.active(True)
    return wlan



json_data = load_data()

if 'ssid' in json_data and 'password' in json_data:
    wifi_ap = connect_ap(json_data['ssid'], json_data['password'])
else:
    wifi_ap = start_ap()

ip = wifi_ap.ifconfig()[0]
print(f'Connected on {ip}')
