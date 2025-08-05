# GPS NTP Server - ハードウェアインターフェース詳細仕様

## 概要

本ドキュメントは、GPS NTP Serverのハードウェアインターフェース詳細仕様を定義します。回路設計、電気的特性、通信プロトコル、および実装ガイドラインを含みます。

## 目次

1. [システム構成](#システム構成)
2. [マイクロコントローラ仕様](#マイクロコントローラ仕様)
3. [通信インターフェース](#通信インターフェース)
4. [センサー・モジュール仕様](#センサーモジュール仕様)
5. [電源設計](#電源設計)
6. [信号整合性](#信号整合性)
7. [PCB設計ガイドライン](#pcb設計ガイドライン)
8. [テスト・検証手順](#テスト検証手順)

## システム構成

### ブロック図

```
                    Raspberry Pi Pico 2 (RP2350)
                   ┌─────────────────────────────┐
    GPS Antenna    │                             │    Ethernet
         │         │   ┌─────────┐   ┌─────────┐ │        │
         └─────────┼──→│GPS/GNSS │   │Ethernet │←┼────────┘
                   │   │ZED-F9T  │   │W5500    │ │
    PPS Signal     │   │         │   │         │ │
         ┌─────────┼──→│   I2C1  │   │   SPI0  │ │
         │         │   └─────────┘   └─────────┘ │
         │         │                             │
    ┌─────────┐    │   ┌─────────┐   ┌─────────┐ │
    │OLED     │    │   │RTC      │   │LEDs &   │ │
    │SH1106   │←───┼───│DS3231   │   │Button   │ │
    │  I2C0   │    │   │  I2C1   │   │  GPIO   │ │
    └─────────┘    │   └─────────┘   └─────────┘ │
                   └─────────────────────────────┘
```

### 主要コンポーネント

| コンポーネント | 型番 | 接続方式 | 説明 |
|---------------|------|---------|------|
| MCU | RP2350 | - | メインコントローラ |
| GPS/GNSS | ZED-F9T | I2C + PPS | 高精度GNSS受信機 |
| Ethernet | W5500 | SPI | イーサネットコントローラ |
| Display | SH1106 | I2C | 128x64 OLED |
| RTC | DS3231 | I2C | 高精度RTC |
| Power | AMS1117-3.3 | - | 3.3V レギュレータ |

## マイクロコントローラ仕様

### RP2350 詳細仕様

```
Processor:
├─ Architecture: ARM Cortex-M33 Dual-Core
├─ Clock Speed: 150MHz (最大)
├─ Flash Memory: 4MB (内蔵)
├─ SRAM: 512KB
├─ GPIO Pins: 30 (26本利用可能)
├─ ADC: 12-bit, 5 channels
├─ PWM: 16 channels
├─ UART: 2 channels
├─ I2C: 2 channels
├─ SPI: 2 channels
└─ USB: 1.1 Full Speed Host/Device

Electrical Characteristics:
├─ Supply Voltage: 3.3V (1.8V - 3.6V)
├─ Operating Temperature: -40°C to +85°C
├─ GPIO Current: 8mA (per pin)
├─ Total GPIO Current: 50mA (maximum)
└─ Power Consumption: 
   ├─ Active: 20mA @ 3.3V
   ├─ Sleep: 0.5mA
   └─ Deep Sleep: 0.1mA
```

### GPIO ピン配置詳細

```
Raspberry Pi Pico 2 Pinout:

         USB       BOOTSEL
         ┌─┐         ╱╲
    GND  │1│      40 │╱ │  VBUS (5V)
    GP0  │2│ I2C0 39 │  │  VSYS (5V)
    GP1  │3│ I2C0 38 │  │  GND
    GND  │4│      37 │  │  3V3_EN
    GP2  │5│      36 │  │  3V3(OUT)
    GP3  │6│      35 │  │  ADC_VREF
    GP4  │7│ LED  34 │  │  GP28 (ADC2)
    GP5  │8│ LED  33 │  │  GND
    GND  │9│      32 │  │  GP27 (ADC1)
    GP6  │10│I2C1 31 │  │  GP26 (ADC0)
    GP7  │11│I2C1 30 │  │  RUN
    GP8  │12│PPS  29 │  │  GP22
    GP9  │13│     28 │  │  GND
    GND  │14│     27 │  │  GP21 (W5500_INT)
    GP10 │15│     26 │  │  GP20 (W5500_RST)
    GP11 │16│BTN  25 │  │  GP19 (SPI0_TX)
    GP12 │17│     24 │  │  GP18 (SPI0_SCK)
    GP13 │18│     23 │  │  GND
    GND  │19│     22 │  │  GP17 (SPI0_CSn)
    GP14 │20│LED  21 │  │  GP16 (SPI0_RX)
         └─┘         │  │
                     ╲  ╱
                      ╲╱
                      LED

Pin Assignment:
├─ I2C0 (OLED): GP0 (SDA), GP1 (SCL)
├─ I2C1 (GPS/RTC): GP6 (SDA), GP7 (SCL)
├─ SPI0 (W5500): GP16 (RX), GP17 (CSn), GP18 (SCK), GP19 (TX)  
├─ PPS Input: GP8
├─ Button Input: GP11
├─ Status LEDs: GP4 (GPS), GP5 (NET), GP14 (ERR), GP15 (PPS)
└─ W5500 Control: GP20 (RST), GP21 (INT)
```

## 通信インターフェース

### I2C インターフェース仕様

#### I2C0 (OLED専用バス)

```
Configuration:
├─ SDA Pin: GP0
├─ SCL Pin: GP1
├─ Clock Speed: 100kHz (標準モード)
├─ Pull-up Resistors: 4.7kΩ (外付け)
└─ Devices: SH1106 OLED (0x3C)

Electrical Characteristics:
├─ VIH (Input High): >2.0V
├─ VIL (Input Low): <0.8V  
├─ VOH (Output High): >2.4V @ 3mA
├─ VOL (Output Low): <0.4V @ 3mA
└─ Input Capacitance: <10pF
```

#### I2C1 (GPS/RTC共有バス)

```
Configuration:
├─ SDA Pin: GP6
├─ SCL Pin: GP7
├─ Clock Speed: 100kHz (標準モード)
├─ Pull-up Resistors: 4.7kΩ (外付け)
└─ Devices: 
   ├─ ZED-F9T GPS (0x42)
   └─ DS3231 RTC (0x68)

Bus Arbitration:
├─ GPS: 定期ポーリング (1Hz)
├─ RTC: 低頻度アクセス
└─ 競合回避: ソフトウェアスケジューリング
```

#### I2C通信プロトコル詳細

```cpp
// I2C Read Transaction
START → SLAVE_ADDR+W → REGISTER_ADDR → 
RESTART → SLAVE_ADDR+R → DATA → NACK → STOP

// I2C Write Transaction  
START → SLAVE_ADDR+W → REGISTER_ADDR → 
DATA → ACK → STOP

// Multi-byte Read Example (GPS)
START → 0x84 → 0xFF → RESTART → 0x85 →
DATA_LEN_H → ACK → DATA_LEN_L → ACK →
DATA[0] → ACK → ... → DATA[n] → NACK → STOP
```

### SPI インターフェース仕様

#### SPI0 (W5500 Ethernet)

```
Configuration:
├─ MISO Pin: GP16
├─ CSn Pin: GP17 (Chip Select)
├─ SCK Pin: GP18 (Clock)
├─ MOSI Pin: GP19
├─ RST Pin: GP20 (Reset)
├─ INT Pin: GP21 (Interrupt)
├─ Clock Speed: 8MHz (maximum)
├─ Mode: SPI Mode 0 (CPOL=0, CPHA=0)
└─ Bit Order: MSB First

Timing Characteristics:
├─ Setup Time: 5ns (minimum)
├─ Hold Time: 5ns (minimum)
├─ Clock High: 60ns (minimum)
├─ Clock Low: 60ns (minimum)
└─ CS Setup: 10ns (minimum)
```

#### W5500 SPI Protocol

```
Command Frame Format:
┌─────────────────────────────────────────────┐
│ Address[15:8] │ Address[7:0] │ Control │ Data │
├───────────────┼──────────────┼─────────┼──────┤
│   8 bits      │   8 bits     │ 8 bits  │ N bytes │
└─────────────────────────────────────────────┘

Control Byte:
├─ Bit[7:3]: Block Select (BSB)
├─ Bit[2]: Read/Write (0=Read, 1=Write)  
├─ Bit[1:0]: Operation Mode
│  ├─ 00: Variable Data Length
│  ├─ 01: Fixed Data Length 1
│  ├─ 10: Fixed Data Length 2
│  └─ 11: Fixed Data Length 4
```

### UART インターフェース (デバッグ用)

```
Configuration:
├─ TX Pin: GP0 (USB CDC)
├─ RX Pin: GP1 (USB CDC)
├─ Baud Rate: 115200 bps
├─ Data Bits: 8
├─ Stop Bits: 1
├─ Parity: None
└─ Flow Control: None
```

## センサー・モジュール仕様

### SparkFun ZED-F9T GPS Module

#### 電気的仕様

```
Power Requirements:
├─ Supply Voltage: 3.3V ±5%
├─ Current Consumption:
│  ├─ Acquisition: 200mA (max)
│  ├─ Tracking: 180mA (typical)
│  ├─ Power Save: 30mA
│  └─ Backup: 10μA
└─ Power-on Time: <1s

Signal Characteristics:
├─ I2C Address: 0x42 (7-bit)
├─ I2C Clock: 100kHz (max 400kHz)
├─ PPS Output:
│  ├─ Voltage: 3.3V CMOS
│  ├─ Pulse Width: 100ms (configurable)
│  ├─ Accuracy: ±20ns (typical)
│  └─ Load Capacity: 50pF
```

#### PPS信号特性

```
PPS Signal Timing:
     ┌─────────────┐
     │             │
─────┘             └─────────────────
     ├─ 100ms ────┤
     ├──────── 1 second ─────────┤

Accuracy Specifications:
├─ Time Pulse Accuracy: ±20ns (1σ)
├─ Frequency Accuracy: ±0.05 ppm
├─ Temperature Stability: ±2.5 ppm (-40°C to +85°C)
└─ Aging: ±1 ppm/year
```

#### アンテナ要件

```
Antenna Specifications:
├─ Frequency: 1575.42MHz (L1), 1176.45MHz (L5)
├─ Gain: >0dBi (minimum)
├─ Axial Ratio: <3dB
├─ VSWR: <2:1
├─ Impedance: 50Ω
└─ Connector: SMA or U.FL

Installation Guidelines:
├─ Ground Plane: >100mm diameter
├─ Height Above Ground: >5mm
├─ Clear Sky View: >120° elevation
├─ Multipath Mitigation: Choke Ring推奨
└─ Cable Length: <10m (RG174)
```

### SH1106 OLED Display

#### 電気的仕様

```
Display Characteristics:
├─ Resolution: 128×64 pixels
├─ Display Area: 30.0×16.0mm
├─ Pixel Pitch: 0.24×0.24mm
├─ Display Mode: Passive Matrix
├─ Driver IC: SH1106
└─ Interface: I2C (0x3C/0x3D)

Power Requirements:
├─ Supply Voltage: 3.3V ±10%
├─ Current Consumption:
│  ├─ Active: 20mA (typical)
│  ├─ Sleep: 10μA
│  └─ Power Off: 1μA
└─ Logic Level: 3.3V CMOS
```

#### 表示制御プロトコル

```cpp
// SH1106 Command Structure
struct SH1106_Command {
    uint8_t co : 1;      // Continuation bit
    uint8_t dc : 1;      // Data/Command bit  
    uint8_t reserved : 6;
    uint8_t data;        // Command or data byte
};

// Initialization Sequence
const uint8_t init_sequence[] = {
    0xAE,        // Display OFF
    0xD5, 0x80,  // Set Display Clock Divide
    0xA8, 0x3F,  // Set Multiplex Ratio
    0xD3, 0x00,  // Set Display Offset
    0x40,        // Set Display Start Line
    0x8D, 0x14,  // Charge Pump Setting
    0x20, 0x00,  // Memory Addressing Mode
    0xA1,        // Set Segment Re-map
    0xC8,        // Set COM Output Scan Direction
    0xDA, 0x12,  // Set COM Pins Configuration
    0x81, 0xCF,  // Set Contrast Control
    0xD9, 0xF1,  // Set Pre-charge Period
    0xDB, 0x40,  // Set VCOMH Deselect Level
    0xA4,        // Entire Display ON
    0xA6,        // Set Normal Display
    0xAF         // Display ON
};
```

### DS3231 RTC Module

#### 電気的仕様

```
Timing Characteristics:
├─ Accuracy: ±2ppm (±63s/year)
├─ Temperature Range: -40°C to +85°C
├─ Crystal: 32.768kHz
├─ Battery Backup: CR2032 (optional)
└─ I2C Address: 0x68

Power Requirements:
├─ Supply Voltage: 2.3V to 5.5V
├─ Active Current: 840μA
├─ Timekeeping Current: 3μA
└─ Battery Current: 840nA
```

### W5500 Ethernet Controller

#### 電気的仕様

```
Network Interface:
├─ IEEE 802.3 10/100 Ethernet
├─ Auto-negotiation
├─ Auto-crossover (MDI/MDIX)
├─ Full/Half Duplex
└─ MAC/PHY integrated

Power Requirements:
├─ Supply Voltage: 3.3V ±5%
├─ Current Consumption:
│  ├─ Active: 150mA (100Mbps)
│  ├─ Link Up: 80mA
│  └─ Power Down: 25mA
```

#### MII信号特性

```
Ethernet Transformer Specifications:
├─ Turn Ratio: 1CT:1CT
├─ Inductance: 350μH (minimum)
├─ DC Resistance: 0.4Ω (maximum)
├─ Isolation: 1500Vrms
└─ Common Mode Choke: 90Ω @ 100MHz

RJ45 Connector Pinout:
├─ Pin 1: TX+ (White/Orange)
├─ Pin 2: TX- (Orange)
├─ Pin 3: RX+ (White/Green)
├─ Pin 4: NC
├─ Pin 5: NC
├─ Pin 6: RX- (Green)
├─ Pin 7: NC
└─ Pin 8: NC
```

## 電源設計

### 電源ブロック図

```
USB-C 5V ──┬── AMS1117-3.3 ──┬── RP2350 (150mA)
           │                 ├── ZED-F9T (180mA)
           │                 ├── W5500 (150mA)
           │                 ├── SH1106 (20mA)
           │                 ├── DS3231 (1mA)
           │                 └── LEDs (30mA)
           │
           └── Protection ──── Fuse (1A)
                Circuit      Overvoltage
                            Protection
```

### 電源仕様

```
Input Power:
├─ Connector: USB-C
├─ Voltage: 5V ±5% (4.75V - 5.25V)
├─ Current: 800mA (maximum)
├─ Power: 4W (maximum)
└─ Protection: Fuse + TVS

3.3V Regulator (AMS1117-3.3):
├─ Input Voltage: 4.75V - 12V
├─ Output Voltage: 3.3V ±2%
├─ Load Regulation: 0.2%
├─ Line Regulation: 0.1%
├─ Dropout Voltage: 1.3V @ 800mA
├─ Current Limit: 1A
└─ Thermal Protection: 125°C

Power Distribution:
├─ Total Current: 531mA (typical)
│  ├─ RP2350: 150mA
│  ├─ ZED-F9T: 180mA
│  ├─ W5500: 150mA
│  ├─ SH1106: 20mA
│  ├─ DS3231: 1mA
│  └─ LEDs: 30mA
├─ Margin: 469mA (47%)
└─ Efficiency: 75% (estimated)
```

### 電源完整性設計

```
Decoupling Capacitors:
├─ Bulk Capacitors:
│  ├─ C1: 220μF (Tantalum, 6.3V)
│  └─ C2: 470μF (Electrolytic, 10V)
├─ Ceramic Capacitors:
│  ├─ C3-C8: 100nF (0603, X7R)
│  └─ C9-C14: 10nF (0603, X7R)
└─ Placement: <5mm from power pins

Power Planes:
├─ 3.3V Plane: Layer 2 (solid pour)
├─ GND Plane: Layer 3 (solid pour)
├─ Via Stitching: 0.2mm diameter, 1mm spacing
└─ Plane Clearance: 0.2mm from traces
```

## 信号整合性

### 高速信号設計

#### SPI信号 (W5500)

```
Trace Specifications:
├─ Impedance: 50Ω ±10%
├─ Trace Width: 0.127mm (5mil)
├─ Via Size: 0.2mm diameter
├─ Layer Stack-up: 4-layer
└─ Dielectric: FR4 (εr=4.3)

Signal Integrity:
├─ Rise Time: <10ns
├─ Crosstalk: <5% (adjacent traces)
├─ Skew: <0.5ns (within group)
└─ Maximum Length: 50mm
```

#### Clock信号設計

```
Clock Distribution:
├─ PPS Signal (1Hz):
│  ├─ Trace Impedance: 50Ω
│  ├─ Maximum Length: 25mm
│  ├─ Termination: 1kΩ pull-down
│  └─ Jitter: <1ns (RMS)
├─ I2C Clocks (100kHz):
│  ├─ Rise Time: <1μs
│  ├─ Fall Time: <300ns
│  ├─ Pull-up: 4.7kΩ to 3.3V
│  └─ Bus Capacitance: <400pF
└─ SPI Clock (8MHz):
   ├─ Duty Cycle: 50% ±5%
   ├─ Jitter: <500ps (RMS)
   └─ Slew Rate: >0.5V/ns
```

### EMI/EMC設計

```
EMI Mitigation:
├─ Ground Planes: Continuous, low impedance
├─ Ferrite Beads: Power and high-speed signals
├─ Shield Can: Optional (GPS module)
└─ Filtering:
   ├─ Common Mode Chokes: USB, Ethernet
   ├─ LC Filters: Power supplies
   └─ RC Filters: Analog signals

EMC Compliance:
├─ FCC Part 15 Class B
├─ CE Mark (EN 55032)
├─ CISPR 32 Class B
└─ VCCI Class B
```

## PCB設計ガイドライン

### 基板構成

```
Layer Stack-up (4-layer, 1.6mm):
├─ L1: Component/Signal (0.035mm copper)
├─ L2: 3.3V Power Plane (0.035mm copper)
├─ L3: Ground Plane (0.035mm copper)
└─ L4: Signal/Component (0.035mm copper)

Dielectric:
├─ Prepreg: 0.2mm (between L1-L2, L3-L4)
├─ Core: 1.0mm (between L2-L3)
└─ Soldermask: 0.025mm (both sides)
```

### コンポーネント配置

```
Component Placement Guidelines:
├─ Power Section:
│  ├─ USB Connector: Edge placement
│  ├─ Regulator: Near input, heat dissipation
│  └─ Bulk Capacitors: Close to regulator
├─ Digital Section:
│  ├─ RP2350: Central location
│  ├─ Crystal: Adjacent to MCU, short traces
│  └─ Decoupling: <5mm from power pins
├─ Analog Section:
│  ├─ GPS Module: Away from switching circuits
│  ├─ RTC: Quiet area, battery backup
│  └─ Clock Sources: Low noise environment
└─ I/O Section:
   ├─ Connectors: Edge placement
   ├─ LEDs: User-visible location
   └─ Button: Accessible position
```

### ルーティング規則

```
Trace Width Rules:
├─ Power (3.3V): 0.254mm (10mil) minimum
├─ High Current (>100mA): 0.381mm (15mil)
├─ Signal Traces: 0.127mm (5mil)
├─ Differential Pairs: 0.127mm (5mil)
└─ Via Size: 0.2mm drill, 0.4mm pad

Spacing Rules:
├─ Trace-to-Trace: 0.127mm (5mil)
├─ Trace-to-Via: 0.1mm (4mil)
├─ Via-to-Via: 0.2mm (8mil)
└─ Component-to-Component: 0.2mm (8mil)
```

### 製造仕様

```
PCB Specifications:
├─ Board Size: 85mm × 55mm (maximum)
├─ Board Thickness: 1.6mm ±10%
├─ Copper Weight: 1oz (35μm)
├─ Surface Finish: HASL or ENIG
├─ Soldermask: Green, matte finish
├─ Silkscreen: White, both sides
└─ Via Fill: Tented or plugged

Manufacturing Tolerances:
├─ Minimum Trace: 0.1mm (4mil)
├─ Minimum Space: 0.1mm (4mil)
├─ Minimum Via: 0.15mm (6mil)
├─ Drill Tolerance: ±0.05mm
└─ Impedance Control: ±10%
```

## テスト・検証手順

### 電気的テスト

#### 電源系統テスト

```
Power Supply Tests:
├─ Input Voltage Range: 4.75V - 5.25V
├─ Output Voltage: 3.3V ±2%
├─ Load Regulation: 0% - 100% load
├─ Line Regulation: Input voltage variation
├─ Ripple & Noise: <50mVpp @ full load
├─ Transient Response: Load step 10% - 90%
├─ Efficiency: Input power vs output power
└─ Thermal Performance: 25°C - 85°C ambient

Test Equipment:
├─ DC Power Supply: Keysight E36313A
├─ Digital Multimeter: Keysight 34461A
├─ Oscilloscope: Keysight DSOX3024T
├─ Electronic Load: Keysight EL34143A
└─ Thermal Camera: FLIR E6
```

#### 通信インターフェーステスト

```
I2C Interface Tests:
├─ Clock Frequency: 100kHz ±1%
├─ Setup/Hold Times: tsu=4.7μs, th=4μs
├─ Rise/Fall Times: tr<1μs, tf<300ns
├─ Bus Capacitance: <400pF
├─ Pull-up Resistance: 4.7kΩ ±5%
└─ Signal Integrity: Eye diagram, jitter

SPI Interface Tests:
├─ Clock Frequency: 8MHz ±1%
├─ Setup/Hold Times: tsu=5ns, th=5ns
├─ Clock Duty Cycle: 50% ±5%
├─ Data Valid Time: tco<25ns
├─ Propagation Delay: tpd<50ns
└─ Signal Quality: Overshoot <10%

Test Procedures:
├─ Loopback Tests: Internal/external
├─ Protocol Compliance: Logic analyzer
├─ Stress Tests: Temperature, voltage
├─ Error Injection: Bit errors, timeouts
└─ Performance: Throughput, latency
```

### 機能テスト

#### GPS機能テスト

```
GPS Performance Tests:
├─ Cold Start TTFF: <30s (outdoor)
├─ Warm Start TTFF: <5s
├─ Hot Start TTFF: <1s
├─ Position Accuracy: <2.5m CEP
├─ Time Accuracy: <100ns (PPS)
├─ Satellite Tracking: >8 satellites
└─ Constellation Support: GPS/GLO/GAL/BDS/QZSS

PPS Signal Verification:
├─ Pulse Width: 100ms ±1ms
├─ Period: 1.000000s ±20ns
├─ Voltage Levels: VIH>2.0V, VIL<0.8V
├─ Rise/Fall Times: <10ns
├─ Jitter: <20ns RMS
└─ Load Driving: 50pF capacitive

Test Environment:
├─ Location: Open sky view
├─ Antenna: Active GPS antenna
├─ Duration: 24 hours minimum
├─ Conditions: -10°C to +60°C
└─ Reference: Cesium frequency standard
```

#### ネットワーク機能テスト

```
Ethernet Tests:
├─ Link Establishment: Auto-negotiation
├─ Speed/Duplex: 10/100 Mbps, Full/Half
├─ Cable Types: CAT5e, CAT6
├─ Cable Length: 1m - 100m
├─ Throughput: UDP/TCP performance
├─ Packet Loss: Error rate measurement
└─ PHY Compliance: IEEE 802.3 tests

NTP Service Tests:
├─ Protocol Compliance: RFC 5905
├─ Stratum Levels: 1 (GPS), 3 (holdover)
├─ Client Compatibility: Windows, Linux, macOS
├─ Load Testing: 100 concurrent clients
├─ Accuracy: ±1ms typical, ±10ms maximum
├─ Stability: Allan deviation measurement
└─ Security: Rate limiting, filtering

Test Tools:
├─ Network Analyzer: Wireshark
├─ NTP Clients: ntpd, chrony, w32time
├─ Load Generator: iperf3, netperf
├─ Time Measurement: GPS-disciplined counter
└─ Protocol Tester: NTP test suite
```

### 環境テスト

#### 温度テスト

```
Temperature Test Profile:
├─ Operating Range: -10°C to +60°C
├─ Storage Range: -40°C to +85°C
├─ Humidity: 85% RH (non-condensing)
├─ Thermal Cycling: -10°C ↔ +60°C (100 cycles)
├─ Soak Time: 2 hours at each extreme
└─ Functional Test: Every 10 cycles

Performance Monitoring:
├─ GPS Accuracy vs Temperature
├─ Crystal Frequency vs Temperature
├─ Power Consumption vs Temperature
├─ Network Performance vs Temperature
└─ Display Performance vs Temperature
```

#### 振動・衝撃テスト

```
Mechanical Tests:
├─ Vibration: IEC 60068-2-6
│  ├─ Frequency: 10Hz - 500Hz
│  ├─ Acceleration: 5g RMS
│  └─ Duration: 2 hours per axis
├─ Shock: IEC 60068-2-27
│  ├─ Peak Acceleration: 100g
│  ├─ Pulse Duration: 6ms
│  └─ Direction: 3 axes, both polarities
└─ Drop Test: 1m height, concrete floor

Pass/Fail Criteria:
├─ No physical damage
├─ Functional operation maintained
├─ Performance within specifications
└─ GPS lock time <2× normal
```

### 製品検証プロトコル

#### インポ検査

```
Incoming Inspection:
├─ Visual Inspection: Component markings, orientation
├─ Electrical Test: In-circuit test (ICT)
├─ Functional Test: Basic operations
├─ Calibration: GPS/RTC time synchronization
└─ Final Test: 24-hour burn-in

Acceptance Criteria:
├─ Zero defects visual
├─ 100% electrical pass
├─ GPS fix <30s cold start
├─ NTP accuracy <10ms
└─ Power consumption <600mA
```

#### 品質保証

```
Quality Metrics:
├─ First Pass Yield: >95%
├─ Field Return Rate: <1%
├─ MTBF: >50,000 hours
├─ Warranty Period: 2 years
└─ RMA Process: 48-hour turnaround

Test Documentation:
├─ Test Plans: Detailed procedures
├─ Test Reports: Results and analysis
├─ Calibration Records: Traceability
├─ Change Control: Version management
└─ Statistical Analysis: Process capability
```

---

## 付録

### 部品表 (BOM)

| Reference | Part Number | Manufacturer | Description | Package | Quantity |
|-----------|-------------|--------------|-------------|---------|----------|
| U1 | RP2350 | Raspberry Pi | Microcontroller | QFN-56 | 1 |
| U2 | ZED-F9T | u-blox | GNSS Module | LGA-54 | 1 |
| U3 | W5500 | WIZnet | Ethernet Controller | LQFP-48 | 1 |
| U4 | SH1106 | Sino Wealth | OLED Controller | COG | 1 |
| U5 | DS3231 | Maxim | RTC | SO-16 | 1 |
| U6 | AMS1117-3.3 | AMS | Voltage Regulator | SOT-223 | 1 |

### 設計チェックリスト

```
Design Review Checklist:
├─ Schematic Review:
│  ├─ □ Power supply design verified
│  ├─ □ Signal integrity analysis complete
│  ├─ □ EMC considerations addressed
│  └─ □ Component selections justified
├─ Layout Review:
│  ├─ □ Component placement optimized
│  ├─ □ Routing rules followed
│  ├─ □ Thermal management adequate
│  └─ □ Manufacturing constraints met
├─ Verification:
│  ├─ □ Simulation results acceptable
│  ├─ □ Design rules check (DRC) clean
│  ├─ □ Electrical rules check (ERC) clean
│  └─ □ Bill of materials complete
└─ Documentation:
   ├─ □ Assembly drawings complete
   ├─ □ Test procedures defined
   ├─ □ User documentation ready
   └─ □ Compliance certifications planned
```

### 参考資料

- RP2350 Datasheet: Raspberry Pi Foundation
- ZED-F9T Integration Manual: u-blox
- W5500 Datasheet: WIZnet
- SH1106 Datasheet: Sino Wealth Electronic
- DS3231 Datasheet: Maxim Integrated
- IPC-2221: Generic Requirements for Printed Board Design
- IPC-6012: Qualification Requirements for Rigid PCB
- IEEE 802.3: Ethernet Standard
- RFC 5905: Network Time Protocol Version 4

---

**Document Version**: 1.0  
**Last Updated**: 2025-07-30  
**Author**: GPS NTP Server Development Team  
**Classification**: Technical Specification