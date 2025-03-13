from machine import PWM, Pin, I2C

import ssd1306
from micropython_pcf8574 import pcf8574
from micropython_scd30.scd30 import SCD30


# senser
i2c_1 = I2C(0,scl=Pin(1), sda=Pin(0))
# remote io
i2c_2 = I2C(1,scl=Pin(7), sda=Pin(6))

scd30 = SCD30(i2c_1, 0x61)


def calc_duty_cycle(color_level:int, brightness: int):
    '''
    color_level: 0 - 100 (Cool daylight <-> Warm white)
    brightness: 0 - 100 (%)
    return c_duty_cycle w_duty_cycle
    '''
    w_duty_cycle = int(65535 * color_level / 100 * (brightness / 100))
    c_duty_cycle = int(65535 * (100 - color_level) / 100 * (brightness / 100))

    return w_duty_cycle, c_duty_cycle


class ControllerMode(object):
    ALL = 0
    LED_1 = 1
    LED_2 = 2


class RemoteController(object):
    def __init__(self, i2c):
        self.display = ssd1306.SSD1306_I2C(128, 32, i2c_2)
        self.display.rotate(False)

        self.pcf = pcf8574.PCF8574(i2c_2, 0x20)
        self.previous_count = 0
        self.previous_pin = 100
        self.press_ignor_time = 2
        self.counter = 0
        self.mode = ControllerMode.ALL

    def _short_press(self, pin):
        if self.pcf.pin(pin) == 1:
            return False

        # ignore short press
        if self.previous_pin == pin and self.counter < self.previous_count + self.press_ignor_time:
            return False
        else:
            self.previous_pin = pin
            self.previous_count = self.counter
            return True

    def _limit(self, value, min_value, max_value):
        if value < min_value:
            return min_value
        elif value > max_value:
            return max_value
        else:
            return value

    def inc_counter(self):
        if self.counter == 2147483647:
            self.counter = 0
        else:
            self.counter +=1

    def display_text(self, s_var):
        head_text = '     cor  bri'
        line1_text = '{0:3d}% {1:3d}%'.format(s_var['led_1']['color_level'], s_var['led_1']['brightness'])
        line2_text = '{0:3d}% {1:3d}%'.format(s_var['led_2']['color_level'], s_var['led_2']['brightness'])

        if self.mode == ControllerMode.LED_1:
            line1_text = '(1) {}'.format(line1_text)
            line2_text = ' 2  {}'.format(line2_text)
        elif self.mode == ControllerMode.LED_2:
            line1_text = ' 1  {}'.format(line1_text)
            line2_text = '(2) {}'.format(line2_text)
        else:
            line1_text = '(1) {}'.format(line1_text)
            line2_text = '(2) {}'.format(line2_text)

        self.display.fill(0)
        self.display.text(head_text, 0, 0)
        self.display.text(line1_text, 0, 10)
        self.display.text(line2_text, 0, 20)
        self.display.show()

    def check_button_events(self, s_var):
        if self._short_press(4):
            print('button ON/OFF pressed')
            if s_var['led_1']['brightness'] >0 or s_var['led_2']['brightness'] >0:
                s_var['led_1']['brightness'] = 0
                s_var['led_2']['brightness'] = 0
            else:
                s_var['led_1']['brightness'] = 100
                s_var['led_2']['brightness'] = 100
            print(s_var)

        if self._short_press(0):
            print('button C- pressed')
            if self.mode == ControllerMode.LED_1 or self.mode == ControllerMode.ALL:
                s_var['led_1']['color_level'] = self._limit(s_var['led_1']['color_level'] - 10, 0, 100)

            if self.mode == ControllerMode.LED_2 or self.mode == ControllerMode.ALL:
                s_var['led_2']['color_level'] = self._limit(s_var['led_2']['color_level'] - 10, 0, 100)

            print(s_var)

        if self._short_press(1):
            print('button C+ pressed')
            if self.mode == ControllerMode.LED_1 or self.mode == ControllerMode.ALL:
                s_var['led_1']['color_level'] = self._limit(s_var['led_1']['color_level'] + 10, 0, 100)

            if self.mode == ControllerMode.LED_2 or self.mode == ControllerMode.ALL:
                s_var['led_2']['color_level'] = self._limit(s_var['led_2']['color_level'] + 10, 0, 100)

            print(s_var)

        if self._short_press(3):
            print('button B- pressed')
            if self.mode == ControllerMode.LED_1 or self.mode == ControllerMode.ALL:
                s_var['led_1']['brightness'] = self._limit(s_var['led_1']['brightness'] - 10, 0, 100)

            if self.mode == ControllerMode.LED_2 or self.mode == ControllerMode.ALL:
                s_var['led_2']['brightness'] = self._limit(s_var['led_2']['brightness'] - 10, 0, 100)

            print(s_var)

        if self._short_press(6):
            print('button B+ pressed')
            if self.mode == ControllerMode.LED_1 or self.mode == ControllerMode.ALL:
                s_var['led_1']['brightness'] = self._limit(s_var['led_1']['brightness'] + 10, 0, 100)

            if self.mode == ControllerMode.LED_2 or self.mode == ControllerMode.ALL:
                s_var['led_2']['brightness'] = self._limit(s_var['led_2']['brightness'] + 10, 0, 100)

            print(s_var)

        if self._short_press(7):
            print('button mode pressed')
            if self.mode == ControllerMode.LED_2:
                self.mode = ControllerMode.ALL
            else:
                self.mode += 1
            print('new mode {}'.format(self.mode))
