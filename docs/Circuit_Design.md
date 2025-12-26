# Hi-Tag 2 Emulator - Circuit Design

## Overview

This document describes the circuit design for the Hi-Tag 2 Emulator, a simplified version of the RFIDler architecture optimized specifically for Hi-Tag 2 transponder emulation.

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

## Circuit Blocks

### 1. Power Supply

The power supply converts 5V from the Arduino/USB to 3.3V for the PIC32 and analog circuits.

**Components:**
- U2: AMS1117-3.3 LDO Regulator
- C1: 22µF input bulk capacitor
- C2: 10µF output bulk capacitor
- C3-C8: 100nF decoupling capacitors (6x)

**Design Notes:**
- AMS1117-3.3 provides up to 1A output current
- Input capacitor stabilizes input voltage
- Output capacitor ensures stable 3.3V rail
- Decoupling capacitors placed near each VDD pin

**Power Consumption:**
| Condition | Current Draw | Power | Notes |
|-----------|--------------|-------|-------|
| Idle (no RF) | ~40-60mA | ~0.15W | PIC32 running, LEDs off |
| Active (RF transmitting) | ~150-200mA | ~0.5-0.7W | Includes RF output driver |
| Programming mode | ~80-100mA | ~0.3W | During ICSP programming |

**Power Supply Requirements:**
- Minimum 5V @ 250mA input recommended
- AMS1117-3.3 dropout voltage: ~1.2V
- USB 2.0 port (500mA) is sufficient
- USB 3.0 port (900mA) provides extra headroom

**Thermal Considerations:**
- AMS1117-3.3 dissipation during RF transmission: P = (5V - 3.3V) × 200mA = 340mW
- SOT-223 package thermal resistance: ~70°C/W
- Expected temperature rise: 340mW × 70°C/W ≈ 24°C above ambient
- No heatsink required for normal operation
- Ensure adequate PCB ground plane for heat dissipation

### 2. PIC32 Microcontroller

The PIC32MX795F512L is the heart of the emulator, providing:
- 80 MHz clock speed for precise timing
- Hardware SPI for Arduino communication
- GPIO for RF control and status

**Key Connections:**
| Function | Pin(s) | Notes |
|----------|--------|-------|
| VDD | 2, 16, 30, 37, 46, 55, 62, 86 | All connected to 3V3 |
| VSS | 15, 31, 36, 45, 65, 75 | All connected to GND |
| MCLR | 13 | Reset with 10kΩ pull-up |
| OSC1/OSC2 | 64, 63 | 8MHz crystal |
| VCAP | 85 | 10µF to GND (required) |

### 3. Crystal Oscillator

The 8 MHz crystal provides the system clock, which the PIC32 PLL multiplies to 80 MHz.

**Components:**
- Y1: 8 MHz crystal (HC49 package)
- C9, C10: 18pF load capacitors

**Design Notes:**
- Load capacitor value calculated from crystal specification
- Keep traces short and away from noisy signals
- Crystal should be as close to PIC32 as possible

### 4. SPI Interface

The SPI interface connects the Arduino Nano bridge to the PIC32.

**Components:**
- R7: 100Ω series resistor on MOSI
- R8: 100Ω series resistor on SCK

**Pin Mapping:**
| Signal | Arduino | PIC32 Pin |
|--------|---------|-----------|
| MOSI | D11 | 25 (SDI1) |
| MISO | D12 | 72 (SDO1) |
| SCK | D13 | 70 (SCK1) |
| CS | D10 | 69 (SS1) |

**Design Notes:**
- Series resistors limit current and reduce ringing
- SPI clock speed: up to 20 MHz
- MISO doesn't need series resistor (output from PIC32)

### 5. Control Signals

Control signals allow the Arduino to manage the PIC32's RF operations.

| Signal | Arduino | PIC32 Pin | Function |
|--------|---------|-----------|----------|
| RESET | D2 | 13 (MCLR) | Hard reset |
| TX_EN | D3 | 17 (RA0) | Enable RF transmission |
| IRQ | D4 | 38 (RA1) | Interrupt from PIC32 |

### 6. RF Output Section

The RF output generates the 125 kHz carrier for transponder emulation.

**Components:**
- R6: 1MΩ bias resistor
- C12, C13, C14: 4.7nF tuning capacitors (3x parallel = 14.1nF)
- L1: 150µH antenna coil

**Design Notes:**
- Resonant frequency: f = 1/(2π√(LC)) ≈ 125 kHz
- Bias resistor sets DC operating point
- Tuning capacitors can be adjusted for exact resonance

### 7. Signal Conditioning (RF Input)

The signal conditioning circuit amplifies and shapes the received RF signal.

**Components:**
- U3: LM358 Dual Op-Amp
- U4: SN74LVC1G17 Schmitt Trigger
- R4: 100kΩ feedback resistor
- R5: 100kΩ input resistor

**Signal Path:**
```
Antenna → R5 → U3 (+) → U3 (out) → U4 (in) → U4 (out) → PIC32 RB4
              ↑                |
              └── R4 ──────────┘ (feedback)
```

**Design Notes:**
- Op-amp gain: Av = R4/R5 = 1 (unity gain buffer)
- Schmitt trigger provides clean digital edges
- Output goes to PIC32 RB4 (pin 12) for RF_IN

### 8. Status LEDs

Two LEDs indicate system status.

| LED | Color | Function | PIC32 Pin |
|-----|-------|----------|-----------|
| LED1 | Green | Power/Status | RB10 (pin 23) |
| LED2 | Red | RF Activity | RB11 (pin 24) |

**Current Calculation:**
- I = (3.3V - Vf) / 330Ω
- Example: I = (3.3V - 2.0V) / 330Ω ≈ 4mA per LED

**Note**: The forward voltage (Vf) varies by LED type and color (typically 1.8-2.2V for red, 2.0-3.0V for green). Always verify Vf from the LED datasheet. If your LEDs have significantly different forward voltages, adjust the current limiting resistor values (R2, R3) accordingly to achieve the desired brightness. For example, a 3.0V green LED would result in I = (3.3V - 3.0V) / 330Ω ≈ 0.9mA, which may be too dim.

### 9. ICSP Programming

The ICSP header (J3) allows in-circuit programming of the PIC32. This is a minimal 3-pin configuration.

| Pin | Signal | PIC32 Pin |
|-----|--------|-----------|
| 1 | MCLR | 13 |
| 2 | PGD | 27 (PGED2) |
| 3 | PGC | 26 (PGEC2) |

**Note**: Power (GND and VDD) must be supplied separately through the main power supply (J1) before programming.

## PCB Design Guidelines

### Layer Stack-up
- Top Layer: Signal routing, components
- Bottom Layer: Ground plane, power routing

### Design Rules
| Parameter | Value |
|-----------|-------|
| Min Track Width | 0.25mm |
| Min Clearance | 0.2mm |
| Via Size | 0.6mm / 0.3mm drill |
| Copper Weight | 1oz |

### Critical Placement
1. **Crystal**: As close to PIC32 OSC pins as possible
2. **Decoupling Caps**: Adjacent to each VDD pin
3. **Antenna**: Away from digital signals
4. **Ground Plane**: Solid pour on bottom layer

## Component Selection

### Why PIC32MX795F512L?
- 80 MHz clock provides 12.5ns resolution
- Sufficient for Hi-Tag 2's 8µs bit timing
- Hardware SPI for efficient communication
- Same MCU as RFIDler (proven design)

### Why LM358?
- Low cost, widely available
- Single supply operation (3.3V)
- Adequate bandwidth for 125 kHz

### Why SN74LVC1G17?
- Schmitt trigger input for noise immunity
- 3.3V compatible
- Fast propagation delay
