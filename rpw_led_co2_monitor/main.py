import uasyncio,time


from machine import Pin, reset, PWM, Timer, soft_reset

from controller import i2c_2, scd30, calc_duty_cycle, RemoteController
from webserver import Webserver

led1_w = PWM(Pin(12), freq=5000, duty_u16=0)
led1_c = PWM(Pin(13), freq=5000, duty_u16=0)
led2_w = PWM(Pin(14), freq=5000, duty_u16=0)
led2_c = PWM(Pin(15), freq=5000, duty_u16=0)

kill_sw = Pin(18, Pin.IN, Pin.PULL_UP)
led = Pin("LED", Pin.OUT)

controller = RemoteController(i2c_2)
timer = Timer()

shared_variables = {
    'scd30': {
        'co2': 0,
        'temperature': 0,
        'humidity': 0,
    },
    'led_1' : {
        'color_level': 50,
        'brightness': 100
    },
    'led_2' : {
        'color_level': 50,
        'brightness': 100
    },
}


async def web_server(reader, writer):
    server = Webserver()
    print('--- client connected ---')

    row_request = await reader.read(4096)

    await writer.awrite(server.get_response(row_request, shared_variables).encode('utf-8'))
    print('Response write')

    await writer.aclose()
    print('Finished request')

def remote_controller(t):
    controller.check_button_events(shared_variables)
    if controller.counter < controller.previous_count + 30:
        controller.display.poweron()
        controller.display_text(shared_variables)
    else:
        controller.display.poweroff()


    if scd30.get_status_ready() == 1:
        ret = scd30.read_measurement()
        shared_variables['scd30']['co2'] = ret[0]
        shared_variables['scd30']['temperature'] = ret[1]
        shared_variables['scd30']['humidity'] = ret[2]
        print(shared_variables)

    w_duty_cycle_1, c_duty_cycle_1 = calc_duty_cycle(
        shared_variables['led_1']['color_level'], shared_variables['led_1']['brightness'])
    led1_w.duty_u16(w_duty_cycle_1)
    led1_c.duty_u16(c_duty_cycle_1)

    w_duty_cycle_2, c_duty_cycle_2 = calc_duty_cycle(
        shared_variables['led_2']['color_level'], shared_variables['led_2']['brightness'])
    led2_w.duty_u16(w_duty_cycle_2)
    led2_c.duty_u16(c_duty_cycle_2)

    controller.inc_counter()


try:
    timer.init(mode=Timer.PERIODIC, period=100, callback=remote_controller)
    event_loop = uasyncio.get_event_loop()
    event_loop.create_task(uasyncio.start_server(web_server, '0.0.0.0', 80))
    event_loop.run_forever()
except KeyboardInterrupt:
    timer.deinit()
except Exception as e:
    timer.deinit()

    import sys
    sys.print_exception(e)

