# Hi-Tag 2 Emulator - Installation Guide

This comprehensive guide covers the installation of all firmware components for the Hi-Tag 2 emulator system.

## System Overview

The Hi-Tag 2 emulator consists of three firmware components:

```
┌─────────────┐     UART      ┌──────────────┐      SPI       ┌─────────────────┐
│ Flipper Zero│◄────────────►│ Arduino Nano │◄──────────────►│ PIC32MX795F512L │
│  (Control)  │   115200     │  (Bridge)    │  4 MHz, Mode 0 │   (RF Engine)   │
└─────────────┘              └──────────────┘                └────────┬────────┘
                                                                       │
                                                               ┌───────▼───────┐
                                                               │  RF Section   │
                                                               │  125kHz Coil  │
                                                               └───────────────┘
```

---

## Prerequisites

### Hardware Requirements

| Component | Specification | Purpose |
|-----------|---------------|---------|
| PIC32MX795F512L | TQFP-100, 80 MHz | RF engine for 125 kHz carrier |
| Arduino Nano 33 BLE Rev2 | nRF52840, 64 MHz | Bridge controller |
| Flipper Zero | Any version | User interface |
| 8 MHz Crystal | ±20 ppm | System clock reference |
| 125 kHz Antenna Coil | ~150 µH | RF field generation |
| USB Cables | 2x (Flipper ↔ Arduino) | Power and data |

### Software Requirements

| Component | Version | Download |
|-----------|---------|----------|
| MPLAB X IDE | 6.00+ | microchip.com/mplab/mplab-x-ide |
| XC32 Compiler | 4.30+ | microchip.com/mplab/xc32 |
| Arduino IDE | 2.0+ | arduino.cc/en/software |
| UFBT (ufbt) | Latest | github.com/flipperdevices/ufbt |
| Python | 3.8+ | python.org |

---

## Part 1: PIC32 Firmware Installation

The PIC32 firmware handles the RF communication - carrier generation, modulation/demodulation, and protocol handling.

### Option A: MPLAB X IDE (GUI)

1. **Install MPLAB X IDE**
   - Download from microchip.com
   - Install with default options
   - Launch MPLAB X IDE

2. **Create New Project**
   ```
   File → New Project
   Choose Project: "Microchip Embedded"
   Select: "32-bit MPLAB Harmony Project"
   Name: "HiTag2_Emulator"
   Location: /path/to/hitag2-emulator
   ```

3. **Configure Device**
   - Select device: `PIC32MX795F512L`
   - Select hardware tool: Your programmer (PICkit 4, ICD 4, etc.)
   - Select compiler: `XC32`

4. **Add Source Files**
   - Right-click `Source Files` → `Add Existing Item`
   - Select all `.c` files from `firmware/pic32/src/`
   - Right-click `Header Files` → `Add Existing Item`
   - Select all `.h` files from `firmware/pic32/include/`

5. **Configure Project Settings**
   ```
   Project Properties → XC32-global
   - Preprocessor Macros: (none required)
   - Include directories: firmware/pic32/include
   - Optimization level: -O2
   ```

6. **Build and Program**
   - Press `Ctrl+F11` to build
   - Press `Ctrl+F10` to program
   - Verify successful programming in output window

### Option B: Command Line (make)

1. **Install XC32 Compiler**
   ```bash
   # Download from Microchip website
   tar -xzf xc32-v4.30-full-install-linux-x64.tar.gz
   sudo ./xc32-v4.30-full-install-linux-x64.install
   ```

2. **Add to PATH**
   ```bash
   export PATH="/opt/microchip/xc32/v4.30/bin:$PATH"
   ```

3. **Build Firmware**
   ```bash
   cd firmware/pic32
   make clean
   make
   ```

   **Output files:**
   - `hitag2_emulator.hex` - Intel hex for programming
   - `hitag2_emulator.elf` - ELF with debug symbols
   - `hitag2_emulator.bin` - Raw binary
   - `hitag2_emulator.map` - Linker map

4. **Program with pic32prog**
   ```bash
   # Install pic32prog
   sudo apt install pic32prog

   # Program (adjust /dev/ttyUSB0 for your programmer)
   pic32prog -d /dev/ttyUSB0 -b 115200 hitag2_emulator.hex
   ```

   **Alternative: MPLAB X IPE**
   ```bash
   # Launch IPE
   /opt/microchip/mplabipe/mplabipe &
   # 1. Select PIC32MX795F512L
   # 2. Connect your programmer
   # 3. Load hitag2_emulator.hex
   # 4. Click "Program"
   ```

### Verify PIC32 Installation

1. **LED Indicators**
   - Status LED (RB10): On when token loaded
   - RF LED (RB11): On when RF field detected

2. **Serial Debug Output**
   ```bash
   # Connect logic analyzer to UART TX (RB8)
   # Or use debug serial if configured
   # Expected output: "Hi-Tag 2 Emulator v1.0 initialized"
   ```

---

## Part 2: Arduino Sketch Installation

The Arduino sketch bridges communication between Flipper Zero (UART) and PIC32 (SPI).

### Installation Steps

1. **Install Arduino IDE**
   ```bash
   # Download from arduino.cc
   tar -xzf arduino-2.0.0-linux64.tar.xz
   sudo mv arduino-2.0.0 /opt/
   /opt/arduino-2.0.0/arduino
   ```

2. **Configure Board Support**
   - Open Arduino IDE
   - Go to `File → Preferences`
   - Add this URL to "Additional Boards Manager URLs":
     ```
     https://raw.githubusercontent.com/arduino/ArduinoCore-nRF528x/master/package_nrf52_index.json
     ```
   - Go to `Tools → Board → Boards Manager`
   - Search for "Arduino nRF528x Boards"
   - Install version 1.3.0+

3. **Select Board and Port**
   ```
   Tools → Board → Arduino Nano 33 BLE
   Tools → Port → /dev/ttyUSB0 (or your Arduino's port)
   ```

4. **Open and Upload Sketch**
   ```
   File → Open
   Navigate to: firmware/arduino/hitag2_arduino.cpp
   Click: Upload (Ctrl+U)
   ```

5. **Verify Installation**
   - Open `Tools → Serial Monitor`
   - Set baud rate to 115200
   - Should see:
     ```
     Hi-Tag 2 Emulator Bridge initialized
     Ready for commands from Flipper Zero
     ```

### Build Specifications

| Parameter | Value |
|-----------|-------|
| Flash Usage | ~32 KB |
| RAM Usage | ~4 KB |
| UART Baud | 115200 |
| SPI Speed | 4 MHz |
| LED Pin | Pin 13 (built-in) |

### Pin Configuration

| Arduino Pin | Function | PIC32 Pin |
|-------------|----------|-----------|
| D10 | SPI SS | Pin 69 (SS1) |
| D11 | SPI MOSI | Pin 25 (SDI1) |
| D12 | SPI MISO | Pin 72 (SDO1) |
| D13 | SPI SCK | Pin 70 (SCK1) |
| D2 | Reset | Pin 13 (MCLR) |
| D3 | TX Enable | Pin 17 (RA0) |
| D4 | IRQ | Pin 38 (RA1) |

---

## Part 3: Flipper Zero Application Installation

The Flipper Zero app provides the user interface for token management and emulation control.

### Option A: Using UFBT (Recommended)

1. **Install UFBT**
   ```bash
   # Linux/macOS
   pip3 install ufbt

   # Or clone and install from source
   git clone https://github.com/flipperdevices/ufbt.git
   cd ufbt
   pip3 install .
   ```

2. **Build Application**
   ```bash
   cd firmware/flipper
   ufbt build
   ```

3. **Install to Flipper Zero**
   ```bash
   # Method 1: Using qFlipper (GUI)
   # 1. Open qFlipper
   # 2. Connect Flipper Zero via USB
   # 3. Click "Install from file"
   # 4. Select hitag2_emulator.fap

   # Method 2: Using ufbt
   ufbt install
   ```

### Option B: Manual Installation

1. **Build with SCons**
   ```bash
   # Set up Flipper SDK
   export FSDK=/path/to/flipper-zero-firmware

   # Build
   cd firmware/flipper
   scons
   ```

2. **Manual Install**
   ```bash
   # Copy FAP to Flipper Zero
   cp firmware/flipper/hitag2_emulator.fap /media/flipper-ufbt/apps/
   # Or use qFlipper for drag-and-drop installation
   ```

### Verify Installation

1. **On Flipper Zero**
   - Power on Flipper Zero
   - Navigate to `Apps → RFID`
   - Should see "Hi-Tag 2 Emulator" icon
   - Tap to launch

2. **Expected Start Screen**
   ```
   ┌──────────────────┐
   │ Hi-Tag 2 Emulator│
   │                  │
   │ Status: Ready    │
   │ Connect Arduino  │
   │                  │
   │ UID: DEADBEEF    │
   │                  │
   │ [OK] Select  [R] │
   └──────────────────┘
   ```

---

## Part 4: Hardware Connections

### SPI Connection (Arduino → PIC32)

```
Arduino Nano 33 BLE          PIC32MX795F512L
┌─────────────┐              ┌─────────────┐
│ D10 (SS)    │──────────────│ 69 (SS1)    │
│ D11 (MOSI)  │──────────────│ 25 (SDI1)   │
│ D12 (MISO)  │──────────────│ 72 (SDO1)   │
│ D13 (SCK)   │──────────────│ 70 (SCK1)   │
│ GND         │──────────────│ GND         │
└─────────────┘              └─────────────┘
```

### Control Signal Connection

```
Arduino Nano 33 BLE          PIC32MX795F512L
┌─────────────┐              ┌─────────────┐
│ D2          │──────────────│ 13 (MCLR)   │
│ D3          │──────────────│ 17 (RA0)    │
│ D4          │──────────────│ 38 (RA1)    │
└─────────────┘              └─────────────┘
```

### USB Connection

```
Flipper Zero ────────────────── Arduino Nano 33 BLE
    USB                            USB
  (OTG)                        (Client)
```

### Power Connections

```
Arduino Nano 33 BLE
┌─────────────────────────────────────────┐
│ VBUS (5V)  ──── Not used               │
│ 3.3V        ──── PIC32 VDD (3.3V)      │
│ GND         ──── PIC32 VSS (GND)       │
└─────────────────────────────────────────┘
```

---

## Part 5: Initial System Test

### 1. Power On Sequence

1. Connect Arduino to USB power
2. Verify Arduino LED (pin 13) is ON
3. Open Serial Monitor at 115200 baud
4. Verify initialization message

### 2. Connect Flipper Zero

1. Connect Flipper Zero to Arduino via USB
2. Launch Hi-Tag 2 Emulator app
3. Verify connection indicator shows "Connected"

### 3. Test Token Loading

1. Select a token from the list
2. Press OK to load
3. Verify UID displays correctly
4. Check Arduino Serial Monitor for confirmation

### 4. Test Emulation Start

1. Press UP arrow to start emulation
2. Verify status changes to "ACTIVE"
3. Check Arduino LED blinks

---

## Build Summary

| Component | Build Command | Output File |
|-----------|---------------|-------------|
| PIC32 | `cd firmware/pic32 && make` | `hitag2_emulator.hex` |
| Arduino | Arduino IDE Upload | `.hex` (internal) |
| Flipper | `cd firmware/flipper && ufbt build` | `hitag2_emulator.fap` |

---

## Next Steps

After successful installation:

1. **Read the [User Guide](User_Guide.md)** for detailed UI operation
2. **Read the [Troubleshooting Guide](Troubleshooting.md)** if issues arise
3. **Consult [HiTag2_Protocol.md](HiTag2_Protocol.md)** for protocol details
4. **Review [Pin_Mapping.md](Pin_Mapping.md)** for hardware connections

---

## Disclaimer

This firmware is for **educational and security research purposes only**. Unauthorized use of RFID emulators may violate laws in your jurisdiction. Always ensure you have proper authorization before testing on access control systems.
