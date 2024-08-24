import time, _thread
from machine import I2C, Pin, reset, PWM
from micropython_scd30.scd30 import SCD30
from webserver import WebserverWithWifi


led = Pin("LED", Pin.OUT)

led1_w = PWM(Pin(12), freq=5000, duty_u16=0)
led1_c = PWM(Pin(13), freq=5000, duty_u16=0)

led2_w = PWM(Pin(14), freq=5000, duty_u16=0)
led2_c = PWM(Pin(15), freq=5000, duty_u16=0)


i2c_1 = I2C(0,scl=Pin(1), sda=Pin(0))
print(i2c_1.scan())

scd30 = SCD30(i2c_1, 0x61)
shared_variables = {
    'scd30': {
        'co2': 0,
        'temperature': 0,
        'humidity': 0,
        },
    'led_1' : {
        'color_level': 0,
        'brightness': 100
        },
    'led_2' : {
        'color_level': 0,
        'brightness': 100
        },
    }

def calc_duty_cycle(color_level:int, brightness: int):
    '''
    color_level: 0 - 100 (Cool daylight <-> Warm white)
    brightness: 0 - 100 (%)
    return c_duty_cycle w_duty_cycle
    '''
    #print('color_level: %s, brightness: %s' % (color_level, brightness))

    w_duty_cycle = int(65535 * color_level / 100 * (brightness / 100))
    c_duty_cycle = int(65535 * (100 - color_level) / 100 * (brightness / 100))

    #print('w_duty_cycle: %s, c_duty_cycle: %s' % (w_duty_cycle, c_duty_cycle))
    return w_duty_cycle, c_duty_cycle

def core0(s_var):
    try:
        webserver = WebserverWithWifi()
        webserver.serve(s_var)
    except Exception as e:
        import sys
        sys.print_exception(e)
        raise e
        

def core1(s_var):
    try:
        while True:
            if scd30.get_status_ready() == 1:
                ret = scd30.read_measurement()
                s_var['scd30']['co2'] = ret[0]
                s_var['scd30']['temperature'] = ret[1]
                s_var['scd30']['humidity'] = ret[2]
                print(s_var)

            w_duty_cycle_1, c_duty_cycle_1 = calc_duty_cycle(s_var['led_1']['color_level']  , s_var['led_1']['brightness'])
            led1_w.duty_u16(w_duty_cycle_1)
            led1_c.duty_u16(c_duty_cycle_1)

            w_duty_cycle_2, c_duty_cycle_2 = calc_duty_cycle(s_var['led_2']['color_level']  , s_var['led_2']['brightness'])
            led2_w.duty_u16(w_duty_cycle_2)
            led2_c.duty_u16(c_duty_cycle_2)

            time.sleep_ms(200)


    except Exception as e:
        import sys
        sys.print_exception(e)
        raise e
        


try:
    _thread.start_new_thread(core1, (shared_variables,))
    core0(shared_variables)
    _thread.exit ()
except KeyboardInterrupt:
    reset()
#except Exception:
#    reset()