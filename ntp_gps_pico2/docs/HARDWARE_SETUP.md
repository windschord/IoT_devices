# GPS NTP Server - Hardware Setup Guide

## Overview

This guide provides step-by-step instructions for assembling and setting up the GPS NTP Server based on Raspberry Pi Pico 2. The system provides high-precision Network Time Protocol (NTP) services using GPS synchronization with PPS (Pulse Per Second) signals.

## Required Components

### Main Components
- **Raspberry Pi Pico 2** (RP2350 microcontroller)
- **SparkFun GNSS Timing Breakout ZED-F9T** (GPS/GNSS receiver)
- **W5500 Ethernet Module** (Network connectivity)
- **SH1106 OLED Display** (128x64 pixels)
- **DS3231 RTC Module** (Real-time clock backup)

### Additional Components
- **GNSS Antenna** (Active antenna recommended for QZSS L1S reception)
- **Breadboard or PCB** (for prototyping/permanent installation)
- **Jumper Wires** (Male-to-Male, Male-to-Female)
- **Pull-up Resistors** (4.7kΩ × 4 for I2C buses)
- **Current Limiting Resistors** (330Ω × 4 for status LEDs)
- **Status LEDs** (Green, Blue, Red, Yellow)
- **Ethernet Cable** (Cat5e or better)
- **Power Supply** (5V/2A USB-C or micro-USB)

## Hardware Connections

### Pin Assignment Summary

```
Raspberry Pi Pico 2 GPIO Pin Assignments:

Power:
- 3V3 (OUT)  → All module VCC pins
- GND        → All module GND pins
- VSYS       → External 5V power input (optional)

I2C Buses:
- GPIO 0 (I2C0 SDA) → SH1106 SDA, DS3231 SDA (with 4.7kΩ pull-up)
- GPIO 1 (I2C0 SCL) → SH1106 SCL, DS3231 SCL (with 4.7kΩ pull-up)
- GPIO 6 (I2C1 SDA) → ZED-F9T SDA (with 4.7kΩ pull-up)
- GPIO 7 (I2C1 SCL) → ZED-F9T SCL (with 4.7kΩ pull-up)

SPI (W5500 Ethernet):
- GPIO 16 (SPI0 RX)  → W5500 MISO
- GPIO 17 (SPI0 CSn) → W5500 CS
- GPIO 18 (SPI0 SCK) → W5500 SCLK
- GPIO 19 (SPI0 TX)  → W5500 MOSI
- GPIO 20            → W5500 RST
- GPIO 21            → W5500 INT

GPS and Control:
- GPIO 8             → ZED-F9T PPS (Pulse Per Second input)
- GPIO 3             → ZED-F9T SAFEBOOT (optional, for firmware updates)

Status LEDs:
- GPIO 4             → Status LED 1 (GNSS Fix) - Green via 330Ω
- GPIO 5             → Status LED 2 (Network) - Blue via 330Ω  
- GPIO 14            → Status LED 3 (Error) - Red via 330Ω
- GPIO 15            → Status LED 4 (PPS) - Yellow via 330Ω
```

### I2C Device Addresses
- **ZED-F9T GNSS Module**: 0x42 (default u-blox address)
- **SH1106 OLED Display**: 0x3C (default OLED address)
- **DS3231 RTC Module**: 0x68 (default RTC address)

## Step-by-Step Assembly

### Step 1: Prepare the Breadboard/PCB

1. **Layout Planning**: Place components to minimize wire crossings
2. **Power Rails**: Connect 3.3V and GND rails across the breadboard
3. **Component Placement**: 
   - Pico 2 in center position
   - GPS module away from switching circuits
   - Ethernet module with adequate spacing
   - OLED display for easy viewing

### Step 2: Install Pull-up Resistors

Install 4.7kΩ pull-up resistors for I2C buses:

```
I2C0 Bus (OLED/RTC):
- 4.7kΩ from GPIO 0 (SDA) to 3.3V
- 4.7kΩ from GPIO 1 (SCL) to 3.3V

I2C1 Bus (GPS):
- 4.7kΩ from GPIO 6 (SDA) to 3.3V  
- 4.7kΩ from GPIO 7 (SCL) to 3.3V
```

### Step 3: Connect Power Distribution

1. **3.3V Distribution**:
   ```
   Pico 3V3 (OUT) → Power Rail → All Module VCC Pins
   ```

2. **Ground Distribution**:
   ```
   Pico GND → Ground Rail → All Module GND Pins
   ```

3. **Power Verification**: Use multimeter to verify 3.3V across all VCC connections

### Step 4: Connect I2C Devices

#### I2C0 Bus (OLED Display & RTC)
```
SH1106 OLED Display:
- VCC → 3.3V rail
- GND → Ground rail  
- SDA → GPIO 0 (with pull-up resistor)
- SCL → GPIO 1 (with pull-up resistor)

DS3231 RTC Module:
- VCC → 3.3V rail
- GND → Ground rail
- SDA → GPIO 0 (shared with OLED)
- SCL → GPIO 1 (shared with OLED)
```

#### I2C1 Bus (GPS Module)
```
ZED-F9T GNSS Module:
- VCC → 3.3V rail
- GND → Ground rail
- SDA → GPIO 6 (with pull-up resistor)
- SCL → GPIO 7 (with pull-up resistor)  
- PPS → GPIO 8 (direct connection)
- SAFEBOOT → GPIO 3 (optional)
```

### Step 5: Connect SPI Ethernet Module

```
W5500 Ethernet Module:
- VCC → 3.3V rail
- GND → Ground rail
- MISO → GPIO 16 (SPI0 RX)
- CS   → GPIO 17 (SPI0 CSn)
- SCLK → GPIO 18 (SPI0 SCK)
- MOSI → GPIO 19 (SPI0 TX)
- RST  → GPIO 20
- INT  → GPIO 21
```

### Step 6: Install Status LEDs

Install 330Ω current limiting resistors for each LED:

```
Status LED Connections:
- LED1 (Green)  → GPIO 4 via 330Ω → LED → GND (GNSS Fix Status)
- LED2 (Blue)   → GPIO 5 via 330Ω → LED → GND (Network Status)
- LED3 (Red)    → GPIO 14 via 330Ω → LED → GND (Error Status)  
- LED4 (Yellow) → GPIO 15 via 330Ω → LED → GND (PPS Status)
```

### Step 7: Connect GNSS Antenna

1. **Antenna Selection**: Use active GNSS antenna for optimal reception
2. **Antenna Placement**: 
   - Clear view of sky (avoid obstructions)
   - Away from interference sources
   - Minimum 1 meter from switching circuits
3. **Connection**: Connect antenna to ZED-F9T ANT connector

### Step 8: Connect Ethernet Cable

1. **Cable Selection**: Use Cat5e or better Ethernet cable
2. **Connection**: Connect to W5500 RJ45 connector
3. **Network Setup**: Connect to router/switch with DHCP or configure static IP

## Power Requirements

### Power Consumption Analysis
- **Raspberry Pi Pico 2**: ~30mA @ 3.3V
- **ZED-F9T GNSS**: ~25mA @ 3.3V (tracking mode)
- **W5500 Ethernet**: ~150mA @ 3.3V (active)
- **SH1106 OLED**: ~20mA @ 3.3V
- **DS3231 RTC**: ~3mA @ 3.3V
- **Status LEDs**: ~20mA @ 3.3V (all on)
- **Total**: ~250mA @ 3.3V

### Power Supply Options

1. **USB Power** (Recommended):
   - Connect USB-C cable to Pico 2 VSYS
   - Use 5V/1A or higher USB power adapter
   - Built-in voltage regulation to 3.3V

2. **External 5V Supply**:
   - Connect 5V to VSYS pin
   - Connect GND to GND pin
   - Use regulated 5V/1A supply

3. **Battery Power** (Portable operation):
   - Use 3×AA battery pack (4.5V) → VSYS
   - Or 4×AA battery pack (6V) → VSYS
   - Operating time: ~8-12 hours depending on capacity

## Signal Quality Considerations

### GPS/GNSS Antenna Placement
- **Location**: Roof-mounted or window-mounted with sky view
- **Orientation**: Vertical polarization (most GNSS antennas)
- **Cable Length**: Minimize cable length to reduce signal loss
- **Interference**: Keep away from 2.4GHz WiFi, cellular towers

### Ethernet Connection
- **Cable Quality**: Use shielded Cat5e/Cat6 for EMI protection
- **Length**: Maximum 100 meters for reliable Gigabit operation
- **Termination**: Ensure proper RJ45 termination

### PPS Signal Integrity
- **Wire Length**: Keep PPS connection as short as possible (<10cm)
- **Shielding**: Use twisted pair or shielded wire for long runs
- **Impedance**: 50Ω characteristic impedance preferred

## Enclosure Recommendations

### Indoor Installation
- **Plastic Enclosure**: IP40 rated minimum
- **Ventilation**: Passive cooling with ventilation slots
- **Mounting**: DIN rail or wall mount brackets
- **Size**: Minimum 150×100×50mm

### Outdoor Installation  
- **Weather Protection**: IP65 rated enclosure minimum
- **Materials**: UV-resistant plastic or aluminum
- **Antenna**: External antenna with weatherproof connection
- **Power**: Weatherproof power connection

## Testing and Verification

### Power-On Test Sequence

1. **Visual Inspection**: Verify all connections before power-on
2. **Power Application**: Connect power and observe LED indicators
3. **Boot Sequence**: 
   - All LEDs should flash during initialization
   - Green (GPS) LED will remain off until satellite acquisition
   - Blue (Network) LED should turn on when Ethernet connects
   - Yellow (PPS) LED should flash once per second after GPS lock

### Connection Verification

1. **I2C Bus Test**:
   ```bash
   # Check OLED display initialization
   # Check RTC communication
   # Check GPS module communication
   ```

2. **Network Test**:
   ```bash
   # Verify DHCP IP assignment
   # Test ping connectivity
   # Check NTP port 123 accessibility
   ```

3. **GPS Test**:
   ```bash
   # Monitor satellite acquisition
   # Verify PPS signal generation
   # Check time synchronization accuracy
   ```

## Safety Considerations

### Electrical Safety
- **ESD Protection**: Use anti-static wrist strap during assembly
- **Power Verification**: Double-check voltages before connecting modules
- **Polarity**: Verify correct polarity for all connections

### RF Safety
- **Antenna Placement**: Follow local RF exposure guidelines
- **Power Levels**: GNSS receive-only, no transmission hazards
- **Grounding**: Ensure proper system grounding for outdoor installations

### Environmental Protection
- **Temperature Range**: -20°C to +70°C operating range
- **Humidity**: <80% relative humidity, non-condensing
- **Altitude**: <3000m for optimal GPS reception

## Next Steps

After completing the hardware assembly:

1. **Firmware Installation**: Flash the GPS NTP server firmware
2. **Initial Configuration**: Configure network and GPS settings
3. **System Testing**: Perform comprehensive system tests
4. **Documentation**: Record configuration settings and test results

Proceed to the [Configuration Guide](CONFIGURATION.md) for software setup instructions.