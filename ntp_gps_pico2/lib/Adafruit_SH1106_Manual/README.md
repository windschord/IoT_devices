# Adafruit SH1106 Library (RP2350 Compatible Version)

## Origin and Credits

This library is based on the following sources:

1. **Original Source**: [wonho-maker/Adafruit_SH1106](https://github.com/wonho-maker/Adafruit_SH1106)
   - Author: wonho-maker
   - Description: Adafruit graphic library for SH1106 driver lcds
   - Based on Adafruit's SSD1306 library, modified for SH1106 controllers

2. **Base Library**: [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
   - Copyright (c) 2012, Adafruit Industries
   - Written by Limor Fried/Ladyada for Adafruit Industries

## Modifications for RP2350 (Raspberry Pi Pico 2) Compatibility

This version includes the following modifications to make it compatible with RP2350 microcontroller:

### Hardware Abstraction Changes
- **AVR Headers**: Added conditional compilation for AVR-specific headers (`util/delay.h`, `avr/pgmspace.h`)
- **Port Manipulation**: Replaced direct port manipulation with `digitalWrite()` calls for RP2040/RP2350
- **SPI Configuration**: Updated SPI clock divider settings for RP2350 compatibility
- **I2C Speed Control**: Made TWBR (TWI Bit Rate Register) AVR-specific with conditional compilation

### Key Technical Changes
```cpp
// Port manipulation (AVR/ESP) -> digitalWrite (RP2040/RP2350)
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO2)
    digitalWrite(cs, HIGH);
    digitalWrite(dc, LOW);
    digitalWrite(cs, LOW);
#else
    *csport |= cspinmask;
    *dcport &= ~dcpinmask;
    *csport &= ~cspinmask;
#endif
```

### Compatibility
- **Supported Platforms**: 
  - Original: AVR, ESP8266, ESP32, SAM
  - Added: RP2040, RP2350 (Raspberry Pi Pico/Pico 2)
- **Display Types**: SH1106 OLED displays (128x64, 128x32, 96x16)
- **Communication**: I2C and SPI interfaces

## Usage

Same as the original Adafruit_SH1106 library. The modifications are transparent to the user API.

```cpp
#include "Adafruit_SH1106.h"

Adafruit_SH1106 display(OLED_RESET);

void setup() {
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello, World!");
  display.display();
}
```

## License

This library maintains the original BSD License from Adafruit Industries.
See LICENSE.txt for full license text.

## Acknowledgments

- **Adafruit Industries**: Original GFX library and hardware inspiration
- **wonho-maker**: SH1106 adaptation of the SSD1306 library
- **Community Contributors**: Various improvements and platform support

---
*Modified for RP2350 compatibility as part of GPS NTP Server project*
*Modifications made in 2024 for Raspberry Pi Pico 2 support*