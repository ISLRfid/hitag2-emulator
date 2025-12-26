# Hi-Tag 2 Emulator - KiCad Project

A simplified 125kHz RFID emulator board designed for Hi-Tag 2 / Paxton NET2 emulation, integrated with Flipper Zero and Arduino Nano.

## Project Overview

| Specification | Value |
|---------------|-------|
| Board Size | 50mm × 40mm |
| Layers | 2 (Top + Bottom) |
| Components | 33 |
| Nets | 27 |
| Estimated Cost | ~$10 (excluding PCB) |

## System Architecture

```
┌─────────────┐     UART      ┌──────────────┐      SPI       ┌─────────────────┐
│ Flipper Zero│◄────────────►│ Arduino Nano │◄──────────────►│   This Board    │
│  (Control)  │   TX/RX      │  (Bridge)    │  MOSI/MISO/SCK │ PIC32MX795F512L │
└─────────────┘              └──────────────┘                └────────┬────────┘
                                                                      │
                                                              ┌───────▼───────┐
                                                              │  125kHz Coil  │
                                                              └───────────────┘
```

## Connector Pinouts

### J1 - SPI Interface (to Arduino)
| Pin | Signal | Arduino Pin | Description |
|-----|--------|-------------|-------------|
| 1 | MOSI | D11 | SPI Master Out |
| 2 | MISO | D12 | SPI Master In |
| 3 | SCK | D13 | SPI Clock |
| 4 | CS | D10 | Chip Select |
| 5 | 5V | 5V | Power Input |
| 6 | GND | GND | Ground |

### J2 - Control Interface (to Arduino)
| Pin | Signal | Arduino Pin | Description |
|-----|--------|-------------|-------------|
| 1 | RESET | D2 | PIC32 Reset |
| 2 | TX_EN | D3 | Transmit Enable |
| 3 | IRQ | D4 | Interrupt Request |
| 4 | GND | GND | Ground |

### J3 - ICSP Programming
| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | MCLR | Master Clear/Reset |
| 2 | PGD | Program Data |
| 3 | PGC | Program Clock |

### J4 - Antenna
| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | COIL_A | Antenna Coil + |
| 2 | GND | Antenna Coil - |

## How to Use

### Step 1: Open the Project
1. Open KiCad 9.0
2. File → Open Project
3. Select `HiTag2_Emulator.kicad_pro`

### Step 2: Import Netlist to PCB
1. Open PCB Editor (Pcbnew)
2. File → Import → Netlist
3. Select `HiTag2_Emulator.net`
4. Click "Update PCB"
5. All footprints will appear with ratsnest showing connections

### Step 3: Place Components
Recommended placement:
- **U1 (PIC32)**: Center of board
- **U2 (LDO)**: Near J1 power pins
- **Y1 (Crystal)**: Adjacent to U1 pins 63/64
- **C9, C10**: Between Y1 and GND
- **J1, J2**: Right edge of board
- **J3**: Top edge (for programming access)
- **J4**: Bottom edge (antenna connection)

### Step 4: Route Traces
Design rules:
- Signal traces: 0.25mm minimum
- Power traces (3V3, GND, VIN): 0.5mm minimum
- Clearance: 0.2mm minimum
- Via size: 0.6mm pad, 0.3mm drill

Routing priority:
1. Power (3V3, GND, VIN) - use wider traces
2. Crystal (OSC1, OSC2) - keep short
3. SPI signals - route parallel, same length
4. RF signals - keep away from digital

### Step 5: Add Ground Plane
1. Select bottom copper layer (B.Cu)
2. Add filled zone covering entire board
3. Assign to GND net
4. This provides return path and shielding

## File Structure

```
HiTag2_Emulator_KiCad/
├── HiTag2_Emulator.kicad_pro    # Project file
├── HiTag2_Emulator.kicad_sch    # Schematic
├── HiTag2_Emulator.kicad_pcb    # PCB layout
├── HiTag2_Emulator.net          # Netlist (import this!)
├── sym-lib-table                # Symbol library reference
├── fp-lib-table                 # Footprint library reference
├── libraries/
│   └── HiTag2_Emulator.kicad_sym  # Custom symbols
├── footprints.pretty/
│   ├── TQFP-100_14x14mm_P0.5mm.kicad_mod
│   ├── SOT-223-3_TabPin2.kicad_mod
│   ├── SOIC-8_3.9x4.9mm_P1.27mm.kicad_mod
│   ├── SOT-23-5.kicad_mod
│   ├── R_0603_1608Metric.kicad_mod
│   ├── C_0805_2012Metric.kicad_mod
│   ├── C_0603_1608Metric.kicad_mod
│   ├── LED_0603_1608Metric.kicad_mod
│   ├── Crystal_HC49-U_Vertical.kicad_mod
│   ├── PinHeader_1x06_P2.54mm_Vertical.kicad_mod
│   ├── PinHeader_1x04_P2.54mm_Vertical.kicad_mod
│   ├── PinHeader_1x03_P2.54mm_Vertical.kicad_mod
│   └── PinHeader_1x02_P2.54mm_Vertical.kicad_mod
└── ../bom/
    └── BOM.csv                  # Bill of Materials
```

## Component Summary

| Category | Count | Components |
|----------|-------|------------|
| ICs | 4 | PIC32, LDO, Op-Amp, Schmitt |
| Capacitors | 14 | Bulk, decoupling, crystal, tuning |
| Resistors | 8 | Pull-up, current limit, feedback |
| LEDs | 2 | Status indicators |
| Crystal | 1 | 8MHz |
| Connectors | 4 | SPI, Control, ICSP, Antenna |
| **Total** | **33** | |

## Antenna Specifications

For 125kHz operation, use:
- **Inductance**: 150-200µH
- **Wire**: 0.2mm enameled copper
- **Turns**: ~100-150 on 50mm diameter form
- **Tuning**: Adjust C12-C14 for resonance

Or use a commercial 125kHz antenna coil.

## Programming

The PIC32 can be programmed via:
1. **ICSP** (J3) using PICkit 3/4 or ICD 4
2. **Bootloader** via SPI (requires bootloader pre-programmed)

## License

This design is based on the RFIDler architecture by Adam Laurie.
Open source for educational and research purposes.
