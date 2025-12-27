# Hi-Tag 2 Emulator - Firmware Documentation

## Overview

This directory contains the complete firmware for the three-tier Hi-Tag 2 emulation system:

1. **PIC32 Firmware** - RF engine for 125 kHz carrier generation and protocol handling
2. **Arduino Sketch** - Bridge controller between Flipper Zero and PIC32
3. **Flipper Zero Application (FAP)** - User interface for token management

## Directory Structure

```
firmware/
├── pic32/
│   ├── src/
│   │   ├── main.c           # Application entry point
│   │   ├── startup.s        # Startup assembly code
│   │   ├── rf_driver.c      # RF modulation/demodulation
│   │   ├── crypto.c         # 48-bit stream cipher
│   │   ├── memory.c         # Tag memory management
│   │   ├── spi_slave.c      # SPI communication
│   │   └── debug.c          # Debug output
│   ├── include/
│   │   ├── main.h
│   │   ├── rf_driver.h
│   │   ├── crypto.h
│   │   ├── memory.h
│   │   ├── spi_slave.h
│   │   └── debug.h
│   ├── Makefile             # Build instructions
│   ├── linker_script.ld     # Memory layout
│   └── README.md            # This file
│
├── arduino/
│   ├── hitag2_arduino.h     # Header file
│   └── hitag2_arduino.cpp   # Bridge controller sketch
│
└── flipper/
    ├── application.fam      # Flipper app manifest
    ├── src/
    │   ├── hitag2_app.c     # Main application
    │   ├── hitag2_scene.c   # Scene management
    │   ├── hitag2_view_main.c
    │   ├── hitag2_view_token_list.c
    │   ├── hitag2_view_token_edit.c
    │   ├── hitag2_view_debug.c
    │   └── hitag2_serial.c  # Serial communication
    └── include/
        ├── hitag2_app.h
        ├── hitag2_scene.h
        ├── hitag2_view.h
        └── hitag2_serial.h
```

## Building the PIC32 Firmware

### Prerequisites

- Microchip MPLAB X IDE (v6.00+)
- XC32 Compiler (v4.30+)
- PIC32 device support package

### Build Steps

1. Open MPLAB X IDE
2. Select File → New Project
3. Choose "Microchip Embedded" → "32-bit MPLAB Harmony Project"
4. Select PIC32MX795F512L as target device
5. Add source files from `firmware/pic32/src/`
6. Add include paths from `firmware/pic32/include/`
7. Build project (Ctrl+F11)

### Command Line Build

```bash
cd firmware/pic32
make
```

Output: `hitag2_emulator.hex`

### Programming

```bash
# Using pic32prog
pic32prog -d /dev/ttyUSB0 -b 115200 hitag2_emulator.hex

# Or using MPLAB X IPE
# Select PIC32MX795F512L
# Load hex file
# Program
```

## Building the Arduino Sketch

### Prerequisites

- Arduino IDE (v2.0+)
- Arduino nRF528x Boards (v1.3.0+)
- Arduino Nano 33 BLE Rev2

### Build Steps

1. Open Arduino IDE
2. Select Tools → Board → "Arduino Nano 33 BLE"
3. Select Tools → Port → (connected port)
4. Open `firmware/arduino/hitag2_arduino.cpp`
5. Click Upload (Ctrl+U)

### Notes

- The sketch uses ~32KB of flash and ~4KB of RAM
- Serial monitor can be used for debugging at 115200 baud
- LED on pin 13 indicates emulation active

## Building the Flipper Zero Application

### Prerequisites

- Flipper Zero SDK (v0.70+)
- Python 3.8+
- scons or ufbt

### Build with UFBT

```bash
cd firmware/flipper
ufbt build
```

### Build with SCons

```bash
# Set up development environment
export FSDK=/path/to/firmware
cd firmware/flipper
scons
```

### Installing to Flipper

1. Copy `.tfw` file to Flipper Zero via qFlipper
2. Or use: `ufbt install`

## Protocol Specifications

### SPI Protocol (Arduino ↔ PIC32)

| Parameter | Value |
|-----------|-------|
| Mode | SPI_MODE_0 (CPOL=0, CPHA=0) |
| Clock | 4 MHz |
| Data Order | MSB first |
| Chip Select | Active low |

### UART Protocol (Flipper ↔ Arduino)

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

### Command Format

```
┌────┬────┬────┬────────┬────────┐
│ SOF│ CMD│LEN │ DATA   │ CHECK  │
│ 0x02│    │    │ LEN B  │ SUM    │
└────┴────┴────┴────────┴────────┘

SOF: Start of frame (0x02)
CMD: Command code
LEN: Data length
DATA: Command data
CHECK: XOR checksum
```

### Supported Commands

| CMD | Name | Description |
|-----|------|-------------|
| 0x01 | PING | Check connection |
| 0x02 | RESET | Reset PIC32 state |
| 0x10 | LOAD_TOKEN | Load 32-byte token |
| 0x11 | SAVE_TOKEN | Get current token |
| 0x12 | LIST_TOKENS | List stored tokens |
| 0x13 | SELECT_TOKEN | Select active token |
| 0x20 | SET_UID | Set 32-bit UID |
| 0x21 | SET_KEY | Set 48-bit key |
| 0x22 | SET_CONFIG | Set configuration |
| 0x30 | GET_STATUS | Get system status |
| 0x40 | START_EMULATE | Start emulation |
| 0x41 | STOP_EMULATE | Stop emulation |

## Hi-Tag 2 Protocol Details

### Physical Layer

- **Frequency**: 125 kHz
- **Bit Rate**: 4 kbps
- **Modulation**: ASK (reader→tag), BPSK (tag→reader)
- **Encoding**: Manchester

### Memory Map (256 bits)

| Page | Size | Description |
|------|------|-------------|
| 0 | 32 bits | UID (read-only) |
| 1 | 32 bits | Configuration |
| 2 | 32 bits | Key bits 0-31 |
| 3 | 32 bits | Key bits 32-47 + Password |
| 4-7 | 32 bits each | User data |

### Authentication

The Hi-Tag 2 uses a challenge-response protocol:

1. Reader sends START_AUTH + 32-bit challenge
2. Tag computes: `Response = LFSR(Key XOR UID XOR Challenge)`
3. Tag sends: `UID[31:0] + Response[31:0]`
4. Reader verifies response using shared key

## Testing

### Hardware Test

1. Connect Arduino to PIC32 via SPI
2. Connect Flipper Zero to Arduino via USB
3. Power on all devices
4. Open Hi-Tag 2 app on Flipper Zero
5. Verify connection status
6. Select or create a token
7. Start emulation
8. Test with compatible reader

### Self-Test Commands

```python
# Python test script example
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200)

# Ping
ser.write(b'\x02\x01\x00\x03\n')
print(ser.readline())

# Read UID
ser.write(b'\x02\x41\x00\x43\n')
print(ser.readline())
```

## Troubleshooting

### PIC32 Issues

| Symptom | Solution |
|---------|----------|
| No response on SPI | Check SS pin connection |
| Carrier not generating | Verify PWM pin (RB5) |
| Memory errors | Check crystal oscillator |

### Arduino Issues

| Symptom | Solution |
|---------|----------|
| Serial not responding | Check USB connection |
| SPI errors | Verify pin connections |
| Token not loading | Check UART communication |

### Flipper Issues

| Symptom | Solution |
|---------|----------|
| App not appearing | Rebuild with correct SDK |
| Connection failed | Check USB serial |
| Display issues | Verify display orientation |

## License

See project root LICENSE file.

## Disclaimer

This firmware is for educational and research purposes only. Unauthorized use of RFID emulators may violate laws.
