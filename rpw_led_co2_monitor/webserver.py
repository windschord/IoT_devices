import network, socket, machine, ujson, ure
from time import sleep

AP_SSID = 'RPW_WIFI'
AP_PASSWORD = '123456789'
AP_RETRY_COUNT = 30

STORE_FILENAME = 'save_data.json'

def load_data():
    json_data = {}
    try:
        with open(STORE_FILENAME, "r") as f:
            json_data = ujson.load(f)
    except (OSError, ValueError):
        with open(STORE_FILENAME, 'w') as f:
            f.write('{}')
    return json_data

def save_data(save_dict: dict):
    json_data = {}

    with open(STORE_FILENAME, "r") as f:
        json_data = ujson.load(f)
        json_data.update(save_dict)
        f.seek(0)
        f.write(ujson.dumps(json_data))
    return json_data

class WebserverWithWifi(object):
    def _connect_ap(self, ssid: str, password: str):
        '''
        Connect to the wifi network

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
            wlan = self._start_ap()
        return wlan


    def _start_ap(self):
        '''
        Start the Access Point
        '''
        print('Starting AP...')
        wlan = network.WLAN(network.AP_IF)
        wlan.config(essid=AP_SSID, password=AP_PASSWORD)
        wlan.active(True)
        return wlan


    def _connect(self):
        '''
        Connect to the wifi network
        '''

        json_data = load_data()

        if 'ssid' in json_data and 'password' in json_data:
            wlan = self._connect_ap(json_data['ssid'], json_data['password'])
        else:
            wlan = self._start_ap()

        ip = wlan.ifconfig()[0]
        print(f'Connected on {ip}')
        return ip

    def _index_page(self, shared_variables):
        '''
        Create a Index page
        '''
        try:
            with open("index.html", "r") as f:
                html = f.read().format(
                    ip=shared_variables['ip'],
                    led1_c=shared_variables['led_1']['color_level'],
                    led1_b=shared_variables['led_1']['brightness'],
                    led2_c=shared_variables['led_2']['color_level'],
                    led2_b=shared_variables['led_2']['brightness'],
                    temperature=shared_variables['scd30']['temperature'],
                    humidity=shared_variables['scd30']['humidity'],
                    co2=shared_variables['scd30']['co2'],
                )
                return str(html)
        except Exception as e:
            import sys
            sys.print_exception(e)
            return f"Error: {e}"

    def _parse_http_request(self, request: bytes):
        '''
        Parse the HTTP request
        '''
        lines = request.split(b"\r\n")
        method, path, _ = lines[0].decode().split(" ")

        headers = {}
        body_pos = 0

        regex = ure.compile(": *")
        for i, line in enumerate(lines[1:]):
            if len(line) < 1:
                body_pos = i + 1
                break
            key, value = regex.split(line.decode(), 1)
            headers[key] = value
        request_body = b''.join(lines[body_pos:])
        return method, path, headers, request_body


    def _route(self, connection, shared_variables):
        '''
        Serve the webpage
        '''
        state = 'OFF'


        client = connection.accept()[0]
        request = client.recv(4096)

        method, path, _, request_body = self._parse_http_request(request)
        print(f'{method}, {path}, {request_body}')


        if method == 'GET' and path == '/lighton?':
            state = 'ON'

        elif method == 'GET' and path == '/lightoff?':
            state = 'OFF'

        elif method == 'POST' and path == '/set_wifi':
            body = {}
            for item in request_body.decode().split('&'):
                k,v = item.split('=', 1)
                body[k] = v

            save_data({'ssid': body['ssid'], 'password': body['password']})

        client.send(self._index_page(shared_variables))
        client.close()


    def _open_socket(self, ip: str):
        '''
        Open a socket on the given IP address
        '''
        address = (ip, 80)
        connection = socket.socket()
        connection.bind(address)
        connection.listen(1)
        return connection

    def serve(self, shared_variables):
        '''
        Serve the webpage
        '''
        ip = self._connect()
        shared_variables['ip'] = ip

        connection = self._open_socket(ip)
        while True:
            if shared_variables['stop']:
                break

            try:
                self._route(connection, shared_variables)
            except Exception as e:
                import sys
                sys.print_exception(e)


#try:
#    ws = WebserverWithWifi()
#    ip = ws.connect()
#    connection = ws.open_socket(ip)
#    ws.serve(connection)

#except KeyboardInterrupt:
#    machine.reset()
#except Exception:
#    machine.reset()
