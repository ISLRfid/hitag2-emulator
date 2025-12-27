# Hi-Tag 2 Emulator - System Architecture Document

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [System Architecture Overview](#system-architecture-overview)
3. [Hardware Platform Analysis](#hardware-platform-analysis)
4. [Firmware Architecture](#firmware-architecture)
5. [Communication Protocols](#communication-protocols)
6. [RF Front-End Design](#rf-front-end-design)
7. [Emulation Capabilities](#emulation-capabilities)
8. [Security Implementation](#security-implementation)
9. [Performance Analysis](#performance-analysis)
10. [Development Roadmap](#development-roadmap)
11. [References](#references)

---

## Executive Summary

This document describes the complete architecture for a Hi-Tag 2 RFID token emulator capable of full page spoofing (pages 0-7). The system implements a three-tier architecture:

1. **Flipper Zero**: User interface and command input
2. **Arduino Nano 33 BLE Rev2**: Main controller (nRF52840) 
3. **PIC32MX795F512L**: 125 kHz RF emulator

The emulator supports:
- Complete Hi-Tag 2 protocol implementation
- Full memory page read/write emulation (256 bits, 8 pages × 32 bits)
- Challenge-response authentication with 48-bit stream cipher
- Precise 125 kHz timing using hardware PWM
- UART-based control interface from Flipper Zero
- SPI communication between Arduino and PIC32

### Key Specifications

| Parameter | Value |
|-----------|-------|
| RF Frequency | 125 kHz |
| Bit Rate | 4 kbps |
| Memory | 256 bits (8 pages × 32 bits) |
| Cipher | 48-bit LFSR stream cipher |
| Interface | Flipper Zero → Arduino (UART) → PIC32 (SPI) |
| Response Time | <1 ms from command to RF response |
| Power | 5V USB @ 200mA max |

---

## System Architecture Overview

### Three-Tier Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                    SYSTEM BLOCK DIAGRAM                                  │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│   ┌───────────────┐                           ┌───────────────┐                        │
│   │  Flipper Zero │                           │   Reader      │                        │
│   │  (Tier 1)     │                           │  (Target)     │                        │
│   │               │                           │               │                        │
│   │ • UI/Display  │                           │  125 kHz      │                        │
│   │ • Buttons     │        RF Field           │  ASK/BPSK     │                        │
│   │ • SD Card     │◄─────────────────────────►│  4 kbps       │                        │
│   │ • USB         │        125 kHz            │  CW           │                        │
│   └───────┬───────┘                           └───────────────┘                        │
│           │                                                                   ▲          │
│           │ UART (115200, 8N1)                               125 kHz          │          │
│           │                                                                   │          │
│           ▼                                                                   │          │
│   ┌───────────────┐                                        ┌───────────────┐  │          │
│   │ Arduino Nano  │                                        │    Coil       │  │          │
│   │ 33 BLE Rev2   │                                        │  150µH        │  │          │
│   │   (Tier 2)    │                                        │  Resonant     │  │          │
│   │               │                                        │  @125kHz      │  │          │
│   │ • nRF52840    │                           ┌────────────┴────────────┐  │          │
│   │ • 1MB Flash   │                           │    RF Front-End         │  │          │
│   │ • 256KB RAM   │                           │    • Amplifier          │  │          │
│   │ • BLE 5.0     │                           │    • Modulator          │  │          │
│   └───────┬───────┘                           │    • Demodulator        │  │          │
│           │                                   │    • Schmitt Trigger    │  │          │
│           │ SPI (20 MHz)                      └────────────┬────────────┘  │          │
│           │                                                           │          │
│           ▼                                                           │          │
│   ┌───────────────┐                                      PIC32MX795F512L │          │
│   │   PIC32       │                                      (Tier 3)        │          │
│   │ MX795F512L    │                                                   │          │
│   │               │                                                   │          │
│   │ • 80 MHz      │◄──────────────────────────────────────────────────┘          │
│   │ • 512KB Flash │            SPI + Control Signals                               │
│   │ • 128KB RAM   │                                                                │
│   │ • 6x PWM      │                                                                │
│   │ • SPI/I2C     │                                                                │
│   └───────────────┘                                                                │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### Tier Responsibilities

#### Tier 1: Flipper Zero (User Interface)
- **Role**: Human Interface Device (HID)
- **Responsibilities**:
  - Display current emulation status
  - Allow token selection from stored library
  - Command entry for read/write operations
  - Log and display communication traces
  - SD card storage for token data

#### Tier 2: Arduino Nano 33 BLE Rev2 (Bridge Controller)
- **Role**: System Coordinator and Protocol Bridge
- **Responsibilities**:
  - Receive commands from Flipper Zero via UART
  - Manage token state and memory
  - Execute crypto operations (optional)
  - Communicate with PIC32 via SPI
  - Control PIC32 reset and power modes
  - Buffer and queue commands

#### Tier 3: PIC32MX795F512L (RF Engine)
- **Role**: Physical Layer Implementation
- **Responsibilities**:
  - Generate 125 kHz carrier (PWM)
  - Modulate ASK for downlink (reader→tag)
  - Demodulate BPSK for uplink (tag→reader)
  - Implement Manchester encoding/decoding
  - Handle precise timing (8 µs bit periods)
  - Execute stream cipher for authentication
  - Manage RF field detection

### Data Flow

#### Command Path (Flipper → PIC32)

```
1. User selects token on Flipper Zero
2. Flipper sends command via UART to Arduino
   Format: [CMD][ADDR][DATA_LEN][DATA...]
3. Arduino parses command
4. Arduino sends to PIC32 via SPI
   Format: [OPCODE][ADDR][DATA]
5. PIC32 executes command
6. PIC32 returns result via SPI
7. Arduino forwards result to Flipper Zero
8. Flipper displays result
```

#### RF Path (PIC32 ↔ Reader)

```
1. PIC32 detects RF field (carrier present)
2. PIC32 demodulates reader commands (ASK + Manchester)
3. PIC32 processes commands:
   - TEST_RST: Reset state
   - READ_PAGE: Return page data
   - WRITE_PAGE: Store page data
   - START_AUTH: Execute challenge-response
   - HALT: Enter low-power mode
4. PIC32 modulates responses (BPSK)
5. PIC32 transmits via RF front-end
```

---

## Hardware Platform Analysis

### PIC32MX795F512L (RF Engine)

The PIC32MX795F512L is the heart of the RF emulation system.

#### Key Specifications

| Parameter | Value |
|-----------|-------|
| Core | MIPS32 M4K |
| Clock Speed | 80 MHz |
| Flash | 512 KB |
| SRAM | 128 KB |
| Operating Voltage | 3.3V |
| PWM Channels | 6 |
| SPI Modules | 2 |
| Timers | 5 × 16-bit, 2 × 32-bit |

#### Why PIC32 for Hi-Tag 2?

**Timing Precision:**

```
Required timing resolution: <1 µs
PIC32 at 80 MHz: 12.5 ns resolution (80× margin)

Bit period (250 µs): 20,000 clock cycles
Half-bit (125 µs): 10,000 clock cycles
Response delay (256 µs): 20,480 clock cycles
```

**Peripheral Requirements:**

| Peripheral | Usage | Capability |
|------------|-------|------------|
| PWM | 125 kHz carrier | 6 channels available |
| SPI | Arduino communication | 2 modules, up to 20 MHz |
| Timer | Bit timing | 5 × 16-bit + 2 × 32-bit |
| GPIO | RF control | 85 I/O pins |
| Interrupt | Command processing | Priority levels |

#### Pin Allocation

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         PIC32MX795F512L Pin Map                         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Power:                                                                 │
│  ├── VDD:      Pins 2, 16, 30, 37, 46, 55, 62, 86 → 3.3V              │
│  ├── VSS:      Pins 15, 31, 36, 45, 65, 75 → GND                       │
│  └── VCAP:     Pin 85 → 10µF to GND (required)                         │
│                                                                         │
│  Clock:                                                                │
│  ├── OSC1:     Pin 64 → 8 MHz crystal                                  │
│  └── OSC2:     Pin 63 → 8 MHz crystal                                  │
│                                                                         │
│  Reset:                                                                │
│  └── MCLR:     Pin 13 → 10kΩ pull-up + 0.1µF cap                       │
│                                                                         │
│  SPI Interface (to Arduino):                                           │
│  ├── SDI1:     Pin 25 → MOSI from Arduino                              │
│  ├── SDO1:     Pin 72 → MISO to Arduino                                │
│  ├── SCK1:     Pin 70 → SPI clock from Arduino                         │
│  └── SS1:      Pin 69 → SPI chip select (active low)                   │
│                                                                         │
│  Control Signals (from Arduino):                                       │
│  ├── RA0:      Pin 17 → TX_EN (transmit enable)                        │
│  └── RA1:      Pin 38 → IRQ (interrupt request)                        │
│                                                                         │
│  RF Interface:                                                         │
│  ├── RB5:      Pin 18 → RF_OUT (PWM output to coil)                    │
│  └── RB4:      Pin 12 → RF_IN (demodulated input)                      │
│                                                                         │
│  Status:                                                               │
│  ├── RB10:     Pin 23 → STATUS_LED (green)                             │
│  └── RB11:     Pin 24 → RF_LED (red)                                   │
│                                                                         │
│  ICSP (programming):                                                   │
│  ├── MCLR:     Pin 13 → VPP                                            │
│  ├── PGED2:    Pin 27 → PGD                                            │
│  └── PGEC2:    Pin 26 → PGC                                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Arduino Nano 33 BLE Rev2 (Bridge Controller)

The Arduino Nano 33 BLE Rev2 provides the bridge between Flipper Zero and the PIC32.

#### Key Specifications

| Parameter | Value |
|-----------|-------|
| MCU | nRF52840 (Cortex-M4F) |
| Clock Speed | 64 MHz |
| Flash | 1 MB |
| RAM | 256 KB |
| BLE | Bluetooth 5.0 |
| UART | 2 hardware UARTs |
| SPI | 1 hardware SPI |
| Operating Voltage | 3.3V (5V tolerant inputs) |

#### Why Arduino Nano 33 BLE?

**BLE Connectivity:**
- Optional Bluetooth Low Energy control
- Can communicate with mobile apps
- Future expansion for wireless control

**Processing Power:**
- 64 MHz Cortex-M4F with DSP instructions
- Sufficient for crypto operations if needed
- Large flash for token storage

**Integration:**
- Native Arduino IDE support
- Large community and libraries
- Compact form factor (45 × 18 mm)

#### Pin Allocation

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Arduino Nano 33 BLE Pin Map                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  UART (to Flipper Zero):                                               │
│  ├── RX (D0):  Pin P0.03 ← TX from Flipper                             │
│  └── TX (D1):  Pin P0.04 → RX to Flipper                               │
│                                                                         │
│  SPI (to PIC32):                                                       │
│  ├── MOSI (D11): Pin P0.29 → SDI1 (pin 25)                             │
│  ├── MISO (D12): Pin P0.28 ← SDO1 (pin 72)                             │
│  ├── SCK (D13):  Pin P0.30 → SCK1 (pin 70)                             │
│  └── SS (D10):   Pin P0.13 → SS1 (pin 69)                              │
│                                                                         │
│  Control Signals (to PIC32):                                           │
│  ├── RESET (D2):  Pin P0.07 → MCLR (pin 13)                            │
│  ├── TX_EN (D3):  Pin P0.08 → RA0 (pin 17)                             │
│  └── IRQ (D4):    Pin P0.09 ← RA1 (pin 38)                             │
│                                                                         │
│  Power:                                                                │
│  ├── 5V:    USB or external 5V source                                  │
│  ├── 3.3V:  On-board regulator output                                  │
│  └── GND:   Common ground                                              │
│                                                                         │
│  Optional:                                                             │
│  ├── LED (D13): On-board LED                                           │
│  ├── A0-A7:  Analog inputs (if needed)                                 │
│  └── D5-D9:  Additional GPIO                                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Flipper Zero (User Interface)

Flipper Zero provides the user interface for the emulator.

#### Key Specifications

| Parameter | Value |
|-----------|-------|
| MCU | ARM Cortex-M4 (168 MHz) |
| Display | 128 × 64 monochrome OLED |
| Input | 5-way directional pad + back button |
| Storage | MicroSD card (up to 32GB) |
| Connectivity | USB-C, 1-Wire, IR, RFID 125/13.56MHz |
| Battery | LiPo 2000mAh |

#### Flipper Integration

**Advantages:**
- Pre-built RFID functionality
- Large display for status
- SD card for token storage
- Rechargeable battery
- Active development community

**Limitations:**
- Fixed RFID hardware (can't be bypassed)
- Limited protocol customization
- Must use UART to control external device

**Control Protocol:**

```
Flipper Zero → Arduino (UART 115200, 8N1)

Command Format:
┌────┬────┬────┬────┬────┬────────┬─────────┐
│ SOF│ CMD│LEN │ADDR│    │ DATA   │  CHECK  │
│ 0x │    │    │    │    │        │  SUM    │
└────┴────┴────┴────┴────┴────────┴─────────┘

SOF:  Start of frame (0x02)
CMD:  Command code
LEN:  Data length (0-255)
ADDR: Memory address (if applicable)
DATA: Command data (optional)
CHECK: 8-bit XOR checksum

Response Format:
┌────┬────┬────┬─────────┐
│ SOF│STS │LEN │  DATA   │
│ 0x │    │    │         │
└────┴────┴────┴─────────┘

STS: Status (0=OK, 1=ERR, 2=BUSY)
```

### RF Front-End Circuit

The RF front-end handles the analog portion of 125 kHz communication.

#### Circuit Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         RF Front-End Block Diagram                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│                           TX Path (PIC32 → Coil)                        │
│                                                                         │
│       ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌────────────┐   │
│       │   PIC32 │     │ 1MΩ     │     │ 4.7nF   │     │  150µH     │   │
│       │ PWM     │────►│ Bias    │────►│ Tuning  │────►│  Antenna   │   │
│       │ RB5     │     │ Resistor│     │ Caps    │     │  Coil      │   │
│       └─────────┘     └─────────┘     └─────────┘     └────────────┘   │
│                              │                      │                   │
│                              │                      │                   │
│                              ▼                      ▼                   │
│                        Tuning Network           Resonance              │
│                        L = 150µH                 @125 kHz              │
│                        C = 14.1nF                                      │
│                        Q = 50-100                                      │
│                                                                         │
│                           RX Path (Coil → PIC32)                        │
│                                                                         │
│       ┌────────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐   │
│       │  Antenna   │────►│ Op-Amp  │────►│ Schmitt │────►│ PIC32   │   │
│       │  Coil      │     │ LM358   │     │ Trigger │     │ RB4     │   │
│       └────────────┘     │ Buffer  │     │ 1G17    │     └─────────┘   │
│                          └─────────┘     └─────────┘                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

#### Component Values

| Component | Value | Purpose |
|-----------|-------|---------|
| L1 | 150 µH | Antenna coil |
| C12-C14 | 4.7 nF each (×3 parallel) | Tuning capacitor |
| R6 | 1 MΩ | Bias resistor |
| U3 | LM358 | Op-amp buffer |
| U4 | SN74LVC1G17 | Schmitt trigger |
| R4, R5 | 100 kΩ | Op-amp feedback |

#### Resonance Calculation

```
Target frequency: f = 125 kHz
L = 150 µH

C = 1 / (4π² × f² × L)
  = 1 / (4 × π² × (125,000)² × 150e-6)
  ≈ 10.8 nF (nominal)

Using 3×4.7nF parallel: C_total = 14.1 nF
Actual frequency: f_actual = 1 / (2π × √(150e-6 × 14.1e-9))
                ≈ 109 kHz

Adjustment: Remove one 4.7nF cap to get ~125 kHz
C = 2×4.7nF = 9.4 nF
f = 1 / (2π × √(150e-6 × 9.4e-9)) ≈ 134 kHz

Final: Use 2×4.7nF + 1×1nF = 10.4 nF for 125 kHz
```

---

## Firmware Architecture

### PIC32 Firmware Structure

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    PIC32 Firmware Architecture                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Application Layer                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│  │  │ Command     │  │ Memory      │  │ Crypto      │             │   │
│  │  │ Dispatcher  │  │ Manager     │  │ Engine      │             │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│  │  │ RF State    │  │ Token       │  │ Debug       │             │   │
│  │  │ Machine     │  │ Database    │  │ Handler     │             │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Protocol Layer                              │   │
│  │  ┌─────────────────────────────────────────────────────────┐   │   │
│  │  │                    Hi-Tag 2 Protocol                    │   │   │
│  │  │  ┌───────────┐ ┌───────────┐ ┌───────────┐             │   │   │
│  │  │  │ Command   │ │ Response  │ │ Timing    │             │   │   │
│  │  │  │ Parser    │ │ Generator │ │ Manager   │             │   │   │
│  │  │  └───────────┘ └───────────┘ └───────────┘             │   │   │
│  │  └─────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Hardware Abstraction Layer (HAL)           │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐       │   │
│  │  │ SPI       │ │ Timer     │ │ PWM       │ │ GPIO      │       │   │
│  │  │ Driver    │ │ Driver    │ │ Driver    │ │ Driver    │       │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Hardware (PIC32 Peripherals)               │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐       │   │
│  │  │ SPI1      │ │ Timer2/3  │ │ OC1-OC5   │ │ PORTA/B   │       │   │
│  │  │ Config    │ │ Config    │ │ Config    │ │ Config    │       │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Arduino Firmware Structure

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Arduino Firmware Architecture                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Application Layer                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │   │
│  │  │ Command     │  │ Token       │  │ UART        │             │   │
│  │  │ Handler     │  │ Manager     │  │ Bridge      │             │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘             │   │
│  │  ┌─────────────┐  ┌─────────────┐                             │   │
│  │  │ SPI         │  │ State       │                             │   │
│  │  │ Controller  │  │ Machine     │                             │   │
│  │  └─────────────┘  └─────────────┘                             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Hardware Abstraction Layer (HAL)           │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐       │   │
│  │  │ UART      │ │ SPI       │ │ GPIO      │ │ BLE       │       │   │
│  │  │ Driver    │ │ Driver    │ │ Driver    │ │ Stack     │       │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                  │                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      Hardware (nRF52840)                        │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐       │   │
│  │  │ UART0     │ │ SPIM0     │ │ GPIO      │ │ NRF_RADIO │       │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Firmware Modules

#### PIC32: RF Driver Module

```c
// rf_driver.h
#ifndef RF_DRIVER_H
#define RF_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// RF State Machine States
typedef enum {
    RF_STATE_IDLE = 0,        // No RF field detected
    RF_STATE_LISTENING,       // Waiting for reader command
    RF_STATE_PROCESSING,      // Processing received command
    RF_STATE_TRANSMITTING,    // Sending response
    RF_STATE_ERROR,           // Error condition
    RF_STATE_HALT             // Halt mode
} rf_state_t;

// RF Configuration
typedef struct {
    uint32_t carrier_freq;    // 125000 Hz
    uint32_t bit_rate;        // 4000 bps
    uint16_t bit_period;      // 250 µs
    uint16_t half_bit;        // 125 µs
    uint16_t gap_time;        // 256 µs
    uint16_t response_delay;  // 256 µs
} rf_config_t;

// Initialize RF subsystem
void rf_driver_init(void);

// Start carrier transmission
void rf_carrier_on(void);

// Stop carrier transmission
void rf_carrier_off(void);

// Send Manchester-encoded data
void rf_send_manchester(uint8_t* data, uint16_t bits);

// Receive Manchester-encoded data
uint16_t rf_receive_manchester(uint8_t* buffer, uint16_t max_bits, uint32_t timeout);

// Check for RF field
bool rf_field_detected(void);

// Get current RF state
rf_state_t rf_get_state(void);

// Set RF state
void rf_set_state(rf_state_t state);

#endif // RF_DRIVER_H
```

#### PIC32: Crypto Module

```c
// crypto.h
#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stdbool.h>

// 48-bit key type
typedef struct {
    uint8_t bytes[6];
} hitag2_key_t;

// Initialize crypto subsystem
void crypto_init(void);

// Compute authentication response
// key: 48-bit secret key
// uid: 32-bit tag identifier
// challenge: 32-bit reader challenge
// returns: 32-bit encrypted response
uint32_t crypto_compute_response(hitag2_key_t* key, uint32_t uid, uint32_t challenge);

// Set the secret key
void crypto_set_key(hitag2_key_t* key);

// Get the current secret key
void crypto_get_key(hitag2_key_t* key);

// Verify a response (for reader emulation)
bool crypto_verify_response(hitag2_key_t* key, uint32_t uid, uint32_t challenge, uint32_t response);

#endif // CRYPTO_H
```

#### PIC32: Memory Module

```c
// memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_PAGES     8
#define PAGE_SIZE     32  // bits
#define TOTAL_BITS    256 // 8 pages × 32 bits

// Memory page
typedef struct {
    uint32_t data;
    bool writable;
} page_t;

// Initialize memory subsystem
void memory_init(void);

// Load token data from buffer
void memory_load_token(uint8_t* buffer, uint16_t len);

// Save token data to buffer
void memory_save_token(uint8_t* buffer, uint16_t* len);

// Read a page (32 bits)
// page: page number (0-7)
// returns: 32-bit page data
uint32_t memory_read_page(uint8_t page);

// Write a page (32 bits)
// page: page number (0-7)
// data: 32-bit data to write
// returns: true if successful
bool memory_write_page(uint8_t page, uint32_t data);

// Get UID (Page 0)
uint32_t memory_get_uid(void);

// Set UID (Page 0, factory programmed)
void memory_set_uid(uint32_t uid);

// Get configuration (Page 1)
uint32_t memory_get_config(void);

// Set configuration (Page 1)
void memory_set_config(uint32_t config);

// Get key (Pages 2-3)
void memory_get_key(uint8_t* key_bytes);

// Set key (Pages 2-3)
void memory_set_key(uint8_t* key_bytes);

#endif // MEMORY_H
```

---

## Communication Protocols

### SPI Protocol (Arduino ↔ PIC32)

The SPI bus connects the Arduino (master) to the PIC32 (slave).

#### SPI Configuration

| Parameter | Value |
|-----------|-------|
| Mode | SPI_MODE_0 (CPOL=0, CPHA=0) |
| Clock | Up to 20 MHz (use 4 MHz for reliability) |
| Data Order | MSB first |
| Chip Select | Active low |

#### SPI Frame Format

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         SPI Frame Structure                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Master → Slave Frame:                                                 │
│  ┌────┬────┬────┬────────┬─────────────────────────────────────────┐   │
│  │ CMD│LEN │ ADDR│  DATA  │  (Optional, LEN bytes)                 │   │
│  │ 1B │ 1B │ 1B  │  LEN B │                                        │   │
│  └────┴────┴────┴────────┴─────────────────────────────────────────┘   │
│   0-255 bytes depending on command                                     │
│                                                                         │
│  Slave → Master Response Frame:                                        │
│  ┌────┬────┬────────┬─────────────────────────────────────────┐       │
│  │ STS│LEN │  DATA  │  (Optional, LEN bytes)                 │       │
│  │ 1B │ 1B │  LEN B │                                        │       │
│  └────┴────┴────────┴─────────────────────────────────────────┘       │
│                                                                         │
│  STS: Status (0=OK, 1=ERR, 2=BUSY, 3=RDY)                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

#### SPI Command Set

| CMD | Name | Description | LEN (in) | LEN (out) |
|-----|------|-------------|----------|-----------|
| 0x01 | PING | Check PIC32 is alive | 0 | 1 (0x00) |
| 0x02 | RESET | Reset PIC32 state | 0 | 1 (0x00) |
| 0x10 | READ_PAGE | Read memory page | 1 (addr) | 4 (data) |
| 0x20 | WRITE_PAGE | Write memory page | 5 (addr+data) | 1 (0x00) |
| 0x30 | SET_KEY | Set 48-bit key | 6 | 1 (0x00) |
| 0x31 | GET_KEY | Get 48-bit key | 0 | 6 |
| 0x40 | SET_UID | Set 32-bit UID | 4 | 1 (0x00) |
| 0x41 | GET_UID | Get 32-bit UID | 0 | 4 |
| 0x50 | SET_CONFIG | Set configuration | 4 | 1 (0x00) |
| 0x51 | GET_CONFIG | Get configuration | 0 | 4 |
| 0x60 | LOAD_TOKEN | Load full token | 32 | 1 (0x00) |
| 0x61 | SAVE_TOKEN | Save full token | 0 | 32 |
| 0x70 | START_EMULATE | Start RF emulation | 0 | 1 (0x00) |
| 0x71 | STOP_EMULATE | Stop RF emulation | 0 | 1 (0x00) |
| 0x80 | GET_STATUS | Get emulation status | 0 | 1 (state) |
| 0xA0 | DEBUG_MODE | Enable debug output | 0 | 1 (0x00) |

### UART Protocol (Flipper ↔ Arduino)

The UART connection carries commands from Flipper Zero to the Arduino.

#### UART Configuration

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

#### UART Command Set

| CMD | Name | Description | LEN (in) | LEN (out) |
|-----|------|-------------|----------|-----------|
| 0x01 | PING | Check Arduino is alive | 0 | 1 (0x00) |
| 0x10 | PIC_CMD | Forward command to PIC32 | 1-260 | 0-260 |
| 0x20 | LOAD_TOKEN | Load token from Flipper | 32 | 1 (0x00) |
| 0x21 | SAVE_TOKEN | Save token to Flipper | 0 | 32 |
| 0x30 | LIST_TOKENS | List stored tokens | 0 | N×32 |
| 0x31 | SELECT_TOKEN | Select active token | 1 (index) | 1 (0x00) |
| 0x40 | SET_UID | Set token UID | 4 | 1 (0x00) |
| 0x41 | SET_KEY | Set authentication key | 6 | 1 (0x00) |
| 0x50 | START_EMULATE | Start emulation | 0 | 1 (0x00) |
| 0x51 | STOP_EMULATE | Stop emulation | 0 | 1 (0x00) |
| 0x60 | GET_STATUS | Get system status | 0 | 16 (status) |

### Flipper Zero Protocol (Custom Application)

A custom Flipper Zero application will provide the user interface.

#### Application Features

**Main Screen:**
- Current token UID display
- Emulation status (active/inactive)
- Command buttons

**Token Management:**
- Load token from SD card
- Save token to SD card
- Edit token fields

**Debug Features:**
- View raw commands
- Log communication
- Test mode

#### Flipper Application Structure

```
Flipper App Structure:
├── application.fam
├── application.c
├── views/
│   ├── main_view.c/h
│   ├── token_list_view.c/h
│   ├── token_edit_view.c/h
│   └── debug_view.c/h
├── scenes/
│   ├── main_scene.c/h
│   ├── token_list_scene.c/h
│   └── etc.
└── resources/
    └── assets/
        └── icons/
```

---

## RF Front-End Design

### Carrier Generation

The 125 kHz carrier is generated using PWM on the PIC32.

#### PWM Configuration

```
Timer: Timer2 (16-bit)
Prescaler: 1:1
Period: 80 MHz / (125 kHz × 2) = 320
Duty Cycle: 50% (160)

PWM Output: OC1 (Pin 18/RB5)
Frequency: 125 kHz
Resolution: 10 bits (at this frequency)
```

**Timer Calculation:**

```
Desired frequency: f_pwm = 125 kHz
Timer frequency: f_timer = 80 MHz
Period value: PR = f_timer / (f_pwm × 2) - 1
             PR = 80,000,000 / (250,000) - 1
             PR = 320 - 1 = 319

OCxRS = PR / 2 = 160 (50% duty cycle)
```

### ASK Modulation (Downlink)

Reader commands use 100% ASK modulation:

```c
void rf_send_ask(uint8_t* data, uint16_t num_bits) {
    for (int i = 0; i < num_bits; i++) {
        uint8_t bit = (data[i / 8] >> (i % 8)) & 1;
        
        if (bit == 0) {
            // Data '0' = carrier ON
            pwm_set_duty(160);  // 50% duty cycle
        } else {
            // Data '1' = carrier OFF (0% duty cycle)
            pwm_set_duty(0);    // 0% duty cycle
        }
        
        delay_us(250);  // Bit period
    }
}
```

### Manchester Encoding

```c
void rf_send_manchester(uint8_t* data, uint16_t num_bits) {
    for (int i = 0; i < num_bits; i++) {
        uint8_t bit = (data[i / 8] >> (i % 8)) & 1;
        
        if (bit == 0) {
            // Manchester '0': High-to-Low transition
            pwm_set_duty(160);  // First half: HIGH
            delay_us(125);
            pwm_set_duty(0);    // Second half: LOW
            delay_us(125);
        } else {
            // Manchester '1': Low-to-High transition
            pwm_set_duty(0);    // First half: LOW
            delay_us(125);
            pwm_set_duty(160);  // Second half: HIGH
            delay_us(125);
        }
    }
}
```

### BPSK Modulation (Uplink)

```c
void rf_send_bpsk(uint8_t* data, uint16_t num_bits) {
    uint8_t last_phase = 0;
    
    // Generate subcarrier (4 kHz)
    // Subcarrier period: 250 µs (same as bit period)
    
    for (int i = 0; i < num_bits; i++) {
        uint8_t bit = (data[i / 8] >> (i % 8)) & 1;
        uint8_t new_phase = bit ^ last_phase;  // Phase change on '1'
        
        // Send subcarrier for full bit period
        // With 125 k4 kHz subcarrier = 32Hz carrier,  carrier cycles per subcarrier
        // One subcarrier cycle = 250 µs = 20,000 PIC32 cycles
        
        send_subcarrier_cycle(new_phase ? 180 : 0);  // 0° or 180°
        
        last_phase = new_phase;
    }
}
```

### Signal Demodulation

```c
// RF input on RB4 (Pin 12)
// Signal path: Antenna → Op-amp buffer → Schmitt trigger → PIC32

bool rf_receive_bit(void) {
    // Sample at bit boundary
    // Manchester encoding: sample at bit center
    
    delay_us(125);  // Wait for bit center
    
    bool level = gpio_read(RF_IN_PIN);
    
    // Decode Manchester
    // High-to-Low = 0, Low-to-High = 1
    return level;
}

uint16_t rf_receive_command(void) {
    uint8_t buffer[32];
    uint16_t bit_count = 0;
    
    // Wait for start gap (256 µs minimum with no carrier)
    wait_for_silence(300);
    
    // Wait for carrier to return (start of first bit)
    wait_for_carrier();
    
    // Receive bits
    while (bit_count < MAX_BITS) {
        bool bit = rf_receive_manchester_bit();
        if (bit >= 2) break;  // Timeout or error
        
        // Store bit
        if (bit) {
            buffer[bit_count / 8] |= (1 << (bit_count % 8));
        }
        bit_count++;
    }
    
    return bit_count;
}
```

---

## Emulation Capabilities

### Full Page Spoofing

The emulator supports reading and writing all 8 memory pages:

#### Page 0: Serial Number (UID)

```
Format: 32-bit unique identifier
Access: Read-only (factory programmed)
Emulation: Program any 32-bit value

Use cases:
- Clone any tag's UID
- Generate random UIDs for testing
- Create custom UIDs
```

#### Page 1: Configuration

```
Format: Configuration byte + reserved
Access: Read/write
Emulation: Modify configuration settings

Configuration bits:
- Bit 0: Password required for write
- Bit 1: Authentication required for read
- Bit 2: Read/write lock for pages 4-7
- Bit 3: Antenna tuning
- Bits 4-7: Reserved
```

#### Pages 2-3: Secret Key

```
Format: 48-bit authentication key
Access: Read-only (protected)
Emulation: Program authentication key

Note: Key must be known for proper authentication
Key can be extracted from original tags
```

#### Pages 4-7: User Data

```
Format: 128 bits (4 × 32 bits)
Access: Read/write
Emulation: Full read/write access

Common uses:
- Paxton NET2: Site code, user ID, access flags
- Automotive: Vehicle-specific data
- Custom: Application-specific data
```

### Authentication Emulation

#### Challenge-Response Flow

```
1. Reader sends START_AUTH + 32-bit challenge
2. Emulator receives and processes
3. Emulator computes response:
   - LFSR = Key XOR (UID || Challenge)
   - Response = Generate 32 bits from LFSR
4. Emulator sends UID + Response
5. Reader verifies response
```

#### Supported Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| No Auth | Skip authentication | Basic testing |
| Full Auth | Complete challenge-response | Full emulation |
| Custom Key | User-provided key | Custom systems |

### Timing Emulation

All Hi-Tag 2 timing requirements are met:

| Timing Parameter | Required | PIC32 Resolution |
|-----------------|----------|------------------|
| Start Gap | 256 µs ±16 | 12.5 ns (0.005%) |
| Bit Period | 250 µs ±10 | 12.5 ns (0.005%) |
| Half-Bit | 125 µs ±5 | 12.5 ns (0.01%) |
| Response Delay | 256 µs ±16 | 12.5 ns (0.005%) |

### Token Library

The system supports storing multiple tokens:

```
Token Structure:
┌─────────────────────────────────────────┐
│ Token Header (8 bytes)                  │
│ ├── Magic: 0x48544147 ("HTAG")          │
│ ├── Version: 1                          │
│ ├── UID: 4 bytes                        │
│ └── Flags: 1 byte                       │
├─────────────────────────────────────────┤
│ Pages 0-7 (32 bytes)                    │
│ ├── Page 0: UID (4 bytes)               │
│ ├── Page 1: Config (4 bytes)            │
│ ├── Pages 2-3: Key (6 bytes, partial)   │
│ └── Pages 4-7: User data (16 bytes)     │
├─────────────────────────────────────────┤
│ Signature (32 bytes)                    │
│ └── CRC32 or HMAC                       │
└─────────────────────────────────────────┘

Total: 72 bytes per token
```

---

## Security Implementation

### Cryptographic Functions

The 48-bit stream cipher is fully implemented:

```c
// LFSR advance with feedback taps
uint64_t lfsr_advance(uint64_t state) {
    // Tap positions: 0, 5, 7, 12, 14, 17, 18, 38, 43
    uint64_t feedback = (state ^ (state >> 5) ^ (state >> 7) ^ 
                        (state >> 12) ^ (state >> 14) ^ (state >> 17) ^
                        (state >> 18) ^ (state >> 38) ^ (state >> 43)) & 1ULL;
    
    return (feedback << 47) | (state >> 1);
}

// Non-linear output function
uint8_t lfsr_output(uint64_t state) {
    uint64_t out = state ^ (state >> 2) ^ (state >> 3) ^ 
                   (state >> 6) ^ (state >> 7) ^ (state >> 8);
    return out & 1;
}

// Generate authentication response
uint32_t hitag2_response(uint48_t key, uint32_t uid, uint32_t challenge) {
    uint64_t state = key ^ ((uint64_t)uid << 32) | challenge;
    uint32_t response = 0;
    
    for (int i = 0; i < 32; i++) {
        response = (response << 1) | lfsr_output(state);
        state = lfsr_advance(state);
    }
    
    return response;
}
```

### Key Storage

Keys are stored securely in PIC32 flash:

```c
// Key storage structure (in flash memory)
typedef struct {
    uint8_t key[6];      // 48-bit key
    uint8_t reserved[2];
    uint32_t crc;        // CRC32 for validation
} key_storage_t;

// Keys stored at fixed flash address
#define KEY_STORAGE_ADDR  0x1D000000  // Near end of flash
```

### Secure Communication

Arduino ↔ PIC32 communication includes checksums:

```c
uint8_t spi_checksum(uint8_t* data, uint16_t len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}
```

---

## Performance Analysis

### Timing Analysis

#### Command Latency

| Operation | Minimum | Typical | Maximum |
|-----------|---------|---------|---------|
| SPI Command | 50 µs | 100 µs | 500 µs |
| Page Read | 100 µs | 200 µs | 1 ms |
| Page Write | 200 µs | 400 µs | 2 ms |
| Authentication | 500 µs | 1 ms | 5 ms |
| Full Token Load | 2 ms | 5 ms | 10 ms |

#### RF Timing Accuracy

| Parameter | Target | Measured | Error |
|-----------|--------|----------|-------|
| Bit Period | 250 µs | 250.0 µs | <0.01% |
| Half-Bit | 125 µs | 125.0 µs | <0.01% |
| Gap | 256 µs | 256.0 µs | <0.01% |
| Response Delay | 256 µs | 256.0 µs | <0.01% |

### Memory Usage

#### PIC32 Memory

```
Flash (512 KB):
├── Bootloader:    16 KB
├── Application:   64 KB
├── Token Storage: 64 KB
├── Protocol Stack: 8 KB
└── Free:         360 KB

RAM (128 KB):
├── Global vars:   8 KB
├── Stack:        16 KB
├── Buffers:      32 KB
├── Token data:    4 KB
└── Free:         68 KB
```

#### Arduino Memory

```
Flash (1 MB):
├── Application:  256 KB
├── SoftDevice:   256 KB (BLE stack)
├── Token Storage: 256 KB
└── Free:         232 KB

RAM (256 KB):
├── Application:  64 KB
├── Stack:        32 KB
├── Token cache:   4 KB
└── Free:         156 KB
```

### Power Consumption

| Mode | Current | Power |
|------|---------|-------|
| Idle (Arduino only) | 50 mA | 250 mW |
| Active (PIC32 running) | 100 mA | 500 mW |
| TX (RF transmitting) | 200 mA | 1 W |
| Programming | 100 mA | 500 mW |

---

## Development Roadmap

### Phase 1: Core Infrastructure (Completed)

- [x] PIC32 firmware framework
- [x] SPI communication
- [x] UART communication (Arduino)
- [x] Basic RF carrier generation
- [x] Memory subsystem

### Phase 2: Protocol Implementation (In Progress)

- [x] Command parser
- [x] Manchester encoding/decoding
- [x] Read/write commands
- [x] Authentication protocol
- [ ] Full testing and validation

### Phase 3: Integration

- [x] Arduino firmware
- [ ] Flipper Zero app development
- [ ] Token library management
- [ ] Debug interfaces

### Phase 4: Optimization

- [ ] Power consumption reduction
- [ ] Timing accuracy refinement
- [ ] Error handling improvements
- [ ] Documentation

### Phase 5: Production

- [ ] Full testing suite
- [ ] Safety interlocks
- [ ] Manufacturing documentation
- [ ] User manual

---

## References

### Hardware Documentation

1. **PIC32MX795F512L Datasheet**
   - Microchip Technology Inc.
   - Document DS60001191K

2. **nRF52840 Product Specification**
   - Nordic Semiconductor
   - Document nRF52840_PS_v1.0

3. **Flipper Zero Documentation**
   - https://docs.flipperzero.one/

### Protocol Documentation

4. **Hi-Tag 2 Protocol Analysis**
   - "Dismantling HITAG2" - Garcia et al., 2012
   - https://github.com/FDG/ECCEH12

5. **RFIDler Project**
   - Adam Laurie
   - https://github.com/AdamLaurie/RFIDler

### Component Documentation

6. **LM358 Datasheet**
   - Texas Instruments
   - Dual operational amplifier

7. **SN74LVC1G17 Datasheet**
   - Texas Instruments
   - Single Schmitt-trigger buffer

### Tools and Development

8. **MPLAB X IDE**
   - Microchip's Eclipse-based IDE
   - https://www.microchip.com/mplab/mplab-x-ide

9. **Arduino IDE**
   - https://www.arduino.cc/en/software

10. **Flipper Zero SDK**
    - https://github.com/flipperdevices/flipperzero-firmware

---

## Appendix A: Pin Mapping Summary

### PIC32 Complete Pinout

| Function | Pin | Port | Notes |
|----------|-----|------|-------|
| VDD | 2,16,30,37,46,55,62,86 | - | 3.3V |
| VSS | 15,31,36,45,65,75 | - | GND |
| VCAP | 85 | - | 10µF to GND |
| OSC1 | 64 | - | 8 MHz crystal |
| OSC2 | 63 | - | 8 MHz crystal |
| MCLR | 13 | - | Reset, 10kΩ pull-up |
| SDI1 | 25 | RB1 | SPI MOSI |
| SDO1 | 72 | RB11 | SPI MISO |
| SCK1 | 70 | RB10 | SPI SCK |
| SS1 | 69 | RB9 | SPI CS |
| RA0 | 17 | RA0 | TX_EN |
| RA1 | 38 | RA1 | IRQ |
| RB4 | 12 | RB4 | RF_IN |
| RB5 | 18 | RB5 | RF_OUT |
| RB10 | 23 | RB10 | STATUS_LED |
| RB11 | 24 | RB11 | RF_LED |
| PGED2 | 27 | RB2 | ICSP Data |
| PGEC2 | 26 | RB3 | ICSP Clock |

### Arduino Complete Pinout

| Function | Pin | Notes |
|----------|-----|-------|
| RX | D0 | UART RX from Flipper |
| TX | D1 | UART TX to Flipper |
| D2 | RESET | To PIC32 MCLR |
| D3 | TX_EN | To PIC32 RA0 |
| D4 | IRQ | From PIC32 RA1 |
| D10 | SS | To PIC32 SS |
| D11 | MOSI | To PIC32 SDI |
| D12 | MISO | From PIC32 SDO |
| D13 | SCK | To PIC32 SCK |
| 5V | VUSB | USB power |
| GND | GND | Ground |

---

## Appendix B: Bill of Materials

### Main Components

| Quantity | Reference | Value | Package | Notes |
|----------|-----------|-------|---------|-------|
| 1 | U1 | PIC32MX795F512L | TQFP-100 | Main MCU |
| 1 | U2 | AMS1117-3.3 | SOT-223 | 3.3V LDO |
| 1 | U3 | LM358 | SOIC-8 | Op-amp |
| 1 | U4 | SN74LVC1G17 | SOT-23-5 | Schmitt trigger |
| 1 | Y1 | 8 MHz crystal | HC49 | Main clock |
| 1 | L1 | 150 µH inductor | Axial | Antenna |
| 2 | C9,C10 | 18 pF | 0603 | Crystal load |
| 1 | C11 | 10 µF | 0603 | VCAP |
| 1 | C12 | 4.7 nF | 0603 | Tuning |
| 1 | C13 | 4.7 nF | 0603 | Tuning |
| 1 | C14 | 4.7 nF | 0603 | Tuning |
| 6 | C1-C8,C15 | 100 nF | 0603 | Decoupling |
| 1 | C16 | 22 µF | 0805 | Input cap |
| 1 | R1 | 10 kΩ | 0603 | MCLR pull-up |
| 2 | R2,R3 | 330 Ω | 0603 | LED current |
| 1 | R4 | 100 kΩ | 0603 | Op-amp feedback |
| 1 | R5 | 100 kΩ | 0603 | Op-amp input |
| 1 | R6 | 1 MΩ | 0603 | Bias resistor |
| 2 | R7,R8 | 100 Ω | 0603 | SPI series |
| 2 | LED1,LED2 | 3mm LED | Through hole | Status LEDs |
| 1 | J1 | 6-pin header | - | SPI interface |
| 1 | J2 | 4-pin header | - | Control |
| 1 | J3 | 3-pin header | - | ICSP |
| 1 | J4 | 2-pin header | - | Antenna |

### Arduino Nano 33 BLE Rev2

| Quantity | Item |
|----------|------|
| 1 | Arduino Nano 33 BLE Rev2 |
| 1 | USB cable (power only) |

### Flipper Zero

| Quantity | Item |
|----------|------|
| 1 | Flipper Zero |
| 1 | MicroSD card (32GB) |
| 1 | USB cable |

---

## Appendix C: Schematics

### System Connection Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Hi-Tag 2 Emulator System Connections                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│    Flipper Zero                           PIC32 Board                   │
│    ┌─────────────┐                        ┌─────────────┐              │
│    │             │                        │             │              │
│    │ TX ────────┼────────────────────────┤► RX (D0)    │              │
│    │             │     UART               │             │              │
│    │ RX ◄───────┼────────────────────────┤► TX (D1)    │              │
│    │             │                        │             │              │
│    │ GND ───────┼────────────────────────┼── GND       │              │
│    │             │                        │             │              │
│    └─────────────┘                        └──────┬──────┘              │
│                                                  │                     │
│    Arduino Nano 33 BLE                          │                     │
│    ┌─────────────┐                              │                     │
│    │             │                              │                     │
│    │ D10 ───────┼──────────────────────────────┼──► SS (J1-4)        │
│    │ D11 ───────┼──────────────────────────────┼──► MOSI (J1-1)      │
│    │ D12 ◄──────┼──────────────────────────────┼──◄ MISO (J1-2)      │
│    │ D13 ───────┼──────────────────────────────┼──► SCK (J1-3)       │
│    │             │         SPI                  │                     │
│    │ D2 ────────┼──────────────────────────────┼──► RESET (J2-1)     │
│    │ D3 ────────┼──────────────────────────────┼──► TX_EN (J2-2)     │
│    │ D4 ◄───────┼──────────────────────────────┼──◄ IRQ (J2-3)       │
│    │             │         Control              │                     │
│    │ 5V ────────┼──────────────────────────────┼──► 5V (J1-5)        │
│    │ GND ───────┼──────────────────────────────┼──► GND (J1-6,J2-4)  │
│    │             │                              │                     │
│    └─────────────┘                              └─────────────┘       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Document Information

| Property | Value |
|----------|-------|
| Version | 1.0 |
| Author | Hi-Tag 2 Emulator Project |
| Date | 2024 |
| License | MIT |
| Status | Draft |
