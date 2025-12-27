# Hi-Tag 2 Emulator

A PIC32MX795F512L-based Hi-Tag 2 RFID emulator designed for Paxton NET2 access control system research and security testing.

![Schematic](images/HiTag2_Schematic_CORRECTED.png)

## Overview

This project implements a hardware emulator for Hi-Tag 2 transponders, specifically targeting Paxton NET2 access control systems. The design is based on the RFIDler architecture but simplified for the specific purpose of Hi-Tag 2 emulation.

### Key Features

- **125 kHz LF RFID** emulation at the hardware level
- **Hi-Tag 2 protocol** support with 48-bit stream cipher
- **Paxton NET2** compatible transponder emulation
- **Flipper Zero + Arduino Nano** control interface
- **PIC32MX795F512L** running at 80 MHz for precise timing
- **Open source** hardware and firmware

## System Architecture

```
┌─────────────┐     UART      ┌──────────────┐      SPI       ┌─────────────────┐
│ Flipper Zero│◄────────────►│ Arduino Nano │◄──────────────►│ PIC32MX795F512L │
│  (Control)  │   TX/RX      │  (Bridge)    │  MOSI/MISO/SCK │   (RF Engine)   │
└─────────────┘              └──────────────┘                └────────┬────────┘
                                                                      │
                                                              ┌───────▼───────┐
                                                              │  RF Section   │
                                                              │  125kHz Coil  │
                                                              └───────────────┘
```

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| MCU | PIC32MX795F512L-80I/PF |
| Clock Speed | 80 MHz |
| Flash | 512 KB |
| RAM | 128 KB |
| Operating Voltage | 3.3V |
| RF Frequency | 125 kHz |
| Board Size | 50mm × 40mm |
| Layers | 2 (Top + Bottom) |

## Repository Structure

```
hitag2-emulator/
├── README.md                 # This file
├── LICENSE                   # MIT License
├── research/                 # HiTag 2 Protocol & Tech Research
├── architecture/             # System Architecture & Capabilities
├── docs/                     # Documentation
│   ├── Circuit_Design.md     # Detailed circuit design document
│   ├── Pin_Mapping.md        # PIC32 pin assignments
│   ├── HiTag2_Protocol.md    # Hi-Tag 2 protocol reference
│   └── Assembly_Guide.md     # PCB assembly instructions
├── hardware/
│   ├── kicad/               # KiCad project files
│   ├── schematics/          # Schematic images (PNG/SVG)
│   └── bom/                 # Bill of Materials
├── firmware/
│   ├── src/                 # Source code
│   └── include/             # Header files
├── tools/                   # Helper scripts and tools
└── images/                  # Project images
```

## Quick Start

### Hardware Requirements

- PIC32MX795F512L-80I/PF (TQFP-100)
- AMS1117-3.3 LDO Regulator
- LM358 Dual Op-Amp
- SN74LVC1G17 Schmitt Trigger
- 8 MHz Crystal + 18pF load capacitors
- 125 kHz antenna coil (150µH)
- Arduino Nano (bridge MCU)
- Flipper Zero (optional, for control)

### Pin Connections

#### SPI Interface (Arduino → PIC32)
| Arduino | PIC32 Pin | Signal |
|---------|-----------|--------|
| D11 (MOSI) | 25 (SDI1) | SPI Data In |
| D12 (MISO) | 72 (SDO1) | SPI Data Out |
| D13 (SCK) | 70 (SCK1) | SPI Clock |
| D10 (SS) | 69 (SS1) | Chip Select |

#### Control Signals
| Arduino | PIC32 Pin | Signal |
|---------|-----------|--------|
| D2 | 13 (MCLR) | Reset |
| D3 | 17 (RA0) | TX Enable |
| D4 | 38 (RA1) | IRQ |

### Building the Firmware

```bash
# Clone the repository
git clone https://github.com/ISLRfid/hitag2-emulator.git
cd hitag2-emulator

# Build with MPLAB X IDE or XC32 compiler
# (Instructions in docs/Building.md)
```

## Documentation

- [HiTag 2 Technology Research](research/HITAG2_Technology_Research.md) - In-depth protocol and technology research
- [System Architecture](architecture/System_Architecture.md) - Detailed hardware and software architecture
- [Circuit Design](docs/Circuit_Design.md) - Detailed explanation of the circuit
- [Pin Mapping](docs/Pin_Mapping.md) - Complete PIC32 pin assignments
- [Hi-Tag 2 Protocol](docs/HiTag2_Protocol.md) - Protocol specification
- [Assembly Guide](docs/Assembly_Guide.md) - PCB assembly instructions
- [Building Guide](docs/Building.md) - Firmware build and programming instructions
- [Bill of Materials](hardware/bom/BOM.csv) - Component list with suppliers

## Schematics

The corrected schematic is available in multiple formats:
- [PNG (High Resolution)](hardware/schematics/HiTag2_Schematic_CORRECTED.png)
- [SVG (Vector)](hardware/schematics/hitag2_schematic_corrected.svg)
- [KiCad Project](hardware/kicad/)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Disclaimer

This project is intended for **educational and security research purposes only**. Users are responsible for ensuring compliance with all applicable laws and regulations. Unauthorized access to access control systems is illegal.

## Acknowledgments

- Based on the [RFIDler](https://github.com/AdamLaurie/RFIDler) architecture by Adam Laurie
- Hi-Tag 2 crypto research by various security researchers
- Flipper Zero community for inspiration

## Contributing

Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

## Contact

For questions and support, please open an issue on GitHub.
