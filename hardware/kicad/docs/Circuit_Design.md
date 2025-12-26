# Hi-Tag 2 Emulator - Simplified Circuit Design

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

## Component List (30 Components Total)

### Integrated Circuits (4)
| Ref | Part | Package | Description |
|-----|------|---------|-------------|
| U1 | PIC32MX795F512L-80I/PF | TQFP-100 | Main MCU, 80MHz, 512KB Flash |
| U2 | AMS1117-3.3 | SOT-223 | 3.3V LDO Regulator |
| U3 | LM358 | SOIC-8 | Dual Op-Amp (signal conditioning) |
| U4 | SN74LVC1G17 | SOT-23-5 | Single Schmitt Trigger |

### Connectors (4)
| Ref | Part | Description |
|-----|------|-------------|
| J1 | Pin Header 1x6 | Arduino SPI Interface |
| J2 | Pin Header 1x4 | Arduino Control Signals |
| J3 | Pin Header 1x3 | ICSP Programming |
| J4 | Pin Header 1x2 | Antenna Coil Connection |

### Passive Components (20)
| Ref | Value | Package | Description |
|-----|-------|---------|-------------|
| C1 | 22µF | 0805 | Input bulk capacitor |
| C2 | 10µF | 0805 | Output bulk capacitor |
| C3-C8 | 100nF | 0603 | Decoupling capacitors (6x) |
| C9 | 18pF | 0603 | Crystal load capacitor |
| C10 | 18pF | 0603 | Crystal load capacitor |
| C11 | 10µF | 0805 | VCAP capacitor |
| C12-C14 | 4.7nF | 0603 | Antenna tuning (3x) |
| R1 | 10kΩ | 0603 | MCLR pull-up |
| R2 | 330Ω | 0603 | LED1 current limit |
| R3 | 330Ω | 0603 | LED2 current limit |
| R4 | 100kΩ | 0603 | Op-amp feedback |
| R5 | 100kΩ | 0603 | Op-amp input |
| R6 | 1MΩ | 0603 | Antenna bias |
| Y1 | 8MHz | HC49 | Main crystal |

### Indicators (2)
| Ref | Part | Description |
|-----|------|-------------|
| LED1 | Green | Power/Status |
| LED2 | Red | RF Activity |

## Schematic Sections

### Section 1: Power Supply
```
VIN (5V from Arduino) ──┬── C1 ──┬── U2 (AMS1117) ──┬── C2 ──┬── 3V3
                        │        │    IN    OUT     │        │
                       GND      GND       GND      GND      GND
```

### Section 2: PIC32 Core
```
                    ┌─────────────────────────────────────┐
                    │         PIC32MX795F512L             │
    3V3 ────────────┤ VDD (pins 2,16,30,37,46,55,62,86)   │
    GND ────────────┤ VSS (pins 15,31,36,45,65,75)        │
                    │                                     │
    Y1.1 ───────────┤ OSC1 (pin 64)                       │
    Y1.2 ───────────┤ OSC2 (pin 63)                       │
                    │                                     │
    VCAP ───────────┤ VCAP (pin 85)                       │
                    │                                     │
    MCLR ───────────┤ MCLR (pin 13)                       │
                    └─────────────────────────────────────┘
```

### Section 3: Arduino SPI Interface (directly from your existing design)
```
J1 Pin Header 1x6:
  Pin 1: MOSI  → PIC32 SDI1 (pin 9) via 100Ω
  Pin 2: MISO  → PIC32 SDO1 (pin 72)
  Pin 3: SCK   → PIC32 SCK1 (pin 70) via 100Ω  
  Pin 4: CS    → PIC32 SS1 (pin 69)
  Pin 5: 5V    → VIN
  Pin 6: GND   → GND

J2 Pin Header 1x4:
  Pin 1: RESET → PIC32 MCLR (pin 13)
  Pin 2: TX_EN → PIC32 RA0 (pin 17)
  Pin 3: IRQ   → PIC32 RA1 (pin 38)
  Pin 4: GND   → GND
```

### Section 4: RF Section (125kHz)
```
                              ┌────────────────┐
    PIC32 RB5 (pin 18) ───────┤ RF_OUT         │
                              │                │
    Tuning Caps C12-C14 ──────┤ COIL_A ────────┼──── J4.1 (to antenna)
                              │                │
                         GND ─┤ COIL_B ────────┼──── J4.2 (to antenna)
                              └────────────────┘

    Antenna receives → U3 (LM358) → U4 (Schmitt) → PIC32 RB4 (pin 17)
```

### Section 5: Signal Conditioning
```
    COIL_A ───┬─── R6 (1M) ───┬─── U3.3 (+IN)
              │               │
             C15             GND
              │
             GND

    U3.1 (OUT) ────── U4.2 (IN) ────── U4.4 (OUT) ────── PIC32.RB4
```

## Net List Summary

| Net Name | Connected Pins |
|----------|----------------|
| VIN | J1.5, C1.1, U2.VIN |
| 3V3 | U2.VOUT, C2.1, C3-C8.1, U1.VDD(all), U3.VCC, U4.VCC, R1.1, LED1.A |
| GND | All ground pins |
| MOSI | J1.1, R7.1 |
| SPI_MOSI | R7.2, U1.9 |
| MISO | J1.2, U1.72 |
| SCK | J1.3, R8.1 |
| SPI_SCK | R8.2, U1.70 |
| CS | J1.4, U1.69 |
| MCLR | J2.1, R1.2, U1.13 |
| TX_EN | J2.2, U1.17 |
| IRQ | J2.3, U1.38 |
| OSC1 | Y1.1, C9.1, U1.64 |
| OSC2 | Y1.2, C10.1, U1.63 |
| VCAP | C11.1, U1.85 |
| RF_OUT | U1.18, Antenna circuit |
| RF_IN | U4.4, U1.17 |
| COIL_A | J4.1, C12-14.1, R6.1 |
| COIL_B | J4.2, GND |
| LED1_K | LED1.C, R2.2 |
| LED2_K | LED2.C, R3.2 |
| STATUS1 | U1.10, R2.1 |
| STATUS2 | U1.11, R3.1 |

## PCB Specifications

| Parameter | Value |
|-----------|-------|
| Board Size | 50mm x 40mm |
| Layers | 2 (Top + Bottom) |
| Min Track | 0.25mm |
| Min Clearance | 0.2mm |
| Via Size | 0.6mm / 0.3mm drill |
| Copper Weight | 1oz |

## Design Notes

1. **Crystal placement**: Keep Y1 and C9/C10 as close to PIC32 pins 63/64 as possible
2. **Decoupling**: Place C3-C8 directly adjacent to each VDD pin
3. **RF section**: Keep antenna traces short and away from digital signals
4. **Ground plane**: Use solid ground pour on bottom layer
5. **SPI traces**: Route as short as possible, keep parallel
