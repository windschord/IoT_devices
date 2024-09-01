import binascii

import machine
import ujson
import ure

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


class Webserver(object):

    def _index_page(self, shared_variables):
        """
        Create Index page
        """
        try:
            with open("index.html", "r") as f:
                html = f.read().format(
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

    def _status_page(self, shared_variables):
        """
        Create Status page
        """
        uuid = str(str(binascii.hexlify(machine.unique_id()), "utf-8"))
        led1_c = shared_variables['led_1']['color_level']
        led1_b = shared_variables['led_1']['brightness']
        led2_c = shared_variables['led_2']['color_level']
        led2_b = shared_variables['led_2']['brightness']
        temperature = shared_variables['scd30']['temperature']
        humidity = shared_variables['scd30']['humidity']
        co2 = shared_variables['scd30']['co2']

        lines = [
            "# HELP led_desk_light desk light.",
            "# TYPE led_desk_light counter",
            f"led_desk_light{{host=\"{uuid}\",id={{led1}},type=color_level}} {led1_c}",
            f"led_desk_light{{host=\"{uuid}\",id={{led1}},type=brightness}} {led1_b}",
            f"led_desk_light{{host=\"{uuid}\",id={{led2}},type=color_level}} {led2_c}",
            f"led_desk_light{{host=\"{uuid}\",id={{led2}},type=brightness}} {led2_b}",
            "",
            "# HELP scd30_temperature temperature.",
            "# TYPE scd30_temperature counter",
            f"scd30_temperature{{host=\"{uuid}\"}} {temperature}",
            "",
            "# HELP scd30_temperature humidity.",
            "# TYPE scd30_humidity counter",
            f"scd30_humidity{{host=\"{uuid}\"}} {humidity}",
            "",
            "# HELP scd30_co2 co2.",
            "# TYPE scd30_co2 co2",
            f"scd30_co2{{host=\"{uuid}\"}} {co2}"
        ]
        return '\n'.join(lines)

    def _parse_http_request(self, request: bytes):
        """
        Parse the HTTP request
        """
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

    def _build_http_response(self, status_code: int = 200, content_type: str = 'text/html', body: str = ''):
        """
        Build the HTTP response
        """
        return f'HTTP/1.1 {status_code}\r\nContent-Type: {content_type}\r\n\r\n{body}'

    def get_response(self, request, shared_variables):
        """
        Serve the webpage
        """
        state = 'OFF'

        method, path, _, request_body = self._parse_http_request(request)
        print(f'{method}, {path}, {request_body}')

        if method == 'GET' and path == '/':
            return self._build_http_response(body=self._index_page(shared_variables))

        elif method == 'GET' and path == '/status':
            return self._build_http_response(body=self._status_page(shared_variables), content_type='text/plain')

        elif method == 'GET' and path == '/lighton?':
            shared_variables['led_1']['brightness'] = 100
            shared_variables['led_2']['brightness'] = 100
            return self._build_http_response(body=self._index_page(shared_variables), status_code=302)

        elif method == 'GET' and path == '/lightoff?':
            shared_variables['led_1']['brightness'] = 0
            shared_variables['led_2']['brightness'] = 0
            return self._build_http_response(body=self._index_page(shared_variables), status_code=302)

        elif method == 'GET' and path == '/styles.css':
            try:
                with open("styles.css", "r") as f:
                    return self._build_http_response(body=f.read(), content_type='text/css')
            except Exception as e:
                import sys
                sys.print_exception(e)
                return self._build_http_response(body=f"Error: {e}", status_code=500)

        elif method == 'POST' and path == '/set_wifi':
            body = {}
            for item in request_body.decode().split('&'):
                k, v = item.split('=', 1)
                body[k] = v

            save_data({'ssid': body['ssid'], 'password': body['password']})
            machine.reset()

        elif method == 'POST' and path == '/change_light':
            body = {}
            for item in request_body.decode().split('&'):
                k, v = item.split('=', 1)
                body[k] = v

            print(body)
            if body.get('led') == '1':
                shared_variables['led_1']['color_level'] = int(body['color'])
                shared_variables['led_1']['brightness'] = int(body['brightness'])

            if body.get('led') == '2':
                shared_variables['led_2']['color_level'] = int(body['color'])
                shared_variables['led_2']['brightness'] = int(body['brightness'])

            return self._build_http_response(body=self._index_page(shared_variables), status_code=302)
        else:
            return self._build_http_response(body="Not found", status_code=404)

        return self._build_http_response(body="Error", status_code=502)
