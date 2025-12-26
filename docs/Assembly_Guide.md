# Hi-Tag 2 Emulator - Assembly Guide

## Overview

This guide covers the assembly of the Hi-Tag 2 Emulator PCB, from component preparation to final testing.

## Required Tools

- Soldering iron (temperature controlled, fine tip)
- Solder (0.5mm diameter, lead-free recommended)
- Flux (no-clean)
- Tweezers (fine tip, ESD safe)
- Magnifying glass or microscope
- Multimeter
- Hot air rework station (optional, for QFP)
- PCB holder or helping hands

## Component Checklist

Before starting, verify you have all components:

### Integrated Circuits
- [ ] U1: PIC32MX795F512L-80I/PF (TQFP-100)
- [ ] U2: AMS1117-3.3 (SOT-223)
- [ ] U3: LM358 (SOIC-8)
- [ ] U4: SN74LVC1G17 (SOT-23-5)

### Passive Components
- [ ] C1: 22µF electrolytic or ceramic (0805)
- [ ] C2: 10µF ceramic (0805)
- [ ] C3-C8: 100nF ceramic (0603) x6
- [ ] C9, C10: 18pF ceramic (0603) x2
- [ ] C11: 10µF ceramic (0805) - VCAP
- [ ] C12-C14: 4.7nF ceramic (0603) x3
- [ ] R1: 10kΩ (0603)
- [ ] R2, R3: 330Ω (0603) x2
- [ ] R4, R5: 100kΩ (0603) x2
- [ ] R6: 1MΩ (0603)
- [ ] R7, R8: 100Ω (0603) x2
- [ ] Y1: 8MHz crystal (HC49)

### Connectors & LEDs
- [ ] J1: 1x6 pin header (2.54mm)
- [ ] J2: 1x4 pin header (2.54mm)
- [ ] J3: 1x3 pin header (2.54mm)
- [ ] J4: 1x2 pin header (2.54mm)
- [ ] LED1: Green LED (0603)
- [ ] LED2: Red LED (0603)
- [ ] L1: 150µH inductor / antenna coil

## Assembly Order

Follow this order to make assembly easier:

### Step 1: SMD Passive Components (0603)

Start with the smallest components:

1. **Decoupling capacitors** (C3-C8): Place near U1 VDD pins
2. **Crystal load capacitors** (C9, C10): Place near crystal footprint
3. **Resistors** (R1-R8): Place according to silkscreen
4. **Tuning capacitors** (C12-C14): Place in RF section

**Tip**: Apply solder paste to one pad, place component with tweezers, reflow, then solder other pad.

### Step 2: SMD Passive Components (0805)

1. **Bulk capacitors** (C1, C2, C11): Larger pads, easier to solder
2. **Verify polarity** on electrolytic capacitors if used

### Step 3: Small ICs

1. **U4 (SN74LVC1G17)**: SOT-23-5 package
   - Align pin 1 with marking on PCB
   - Tack one corner, align, solder remaining pins

2. **U3 (LM358)**: SOIC-8 package
   - Pin 1 indicated by dot/notch
   - Tack pins 1 and 8, then solder remaining

3. **U2 (AMS1117-3.3)**: SOT-223 package
   - Large tab is output (pin 2)
   - Ensure good thermal connection to PCB

### Step 4: PIC32 Microcontroller (U1)

The TQFP-100 package requires careful attention:

**Method 1: Drag Soldering**
1. Apply flux to all pads
2. Align IC carefully (pin 1 marker)
3. Tack opposite corners
4. Verify alignment under magnification
5. Apply solder to iron tip
6. Drag along pins, flux will prevent bridges
7. Clean and inspect for bridges
8. Use solder wick to remove any bridges

**Method 2: Hot Air**
1. Apply solder paste to pads
2. Place IC with tweezers
3. Apply hot air at 350°C
4. Watch for solder to reflow
5. Remove heat, let cool
6. Inspect for bridges

### Step 5: Crystal

1. Place Y1 (8MHz crystal) in footprint
2. Solder both pads
3. Keep leads short

### Step 6: LEDs

**Important**: Observe polarity!
- Cathode (negative) usually marked with line/dot
- Anode connects to resistor side

1. LED1 (Green): Status indicator
2. LED2 (Red): RF activity

### Step 7: Through-Hole Components

1. **Pin headers** (J1-J4): 
   - Insert from top
   - Solder from bottom
   - Keep perpendicular to board

2. **Antenna coil** (L1):
   - If using external coil, solder wires to J4
   - If using PCB coil, connect jumper

## Post-Assembly Inspection

### Visual Inspection

- [ ] No solder bridges between IC pins
- [ ] All components present and correctly oriented
- [ ] Solder joints shiny and well-formed
- [ ] No cold joints (dull, grainy appearance)
- [ ] LEDs oriented correctly

### Electrical Testing

**Before applying power:**

1. **Check for shorts**:
   - Measure resistance between VDD and GND
   - Should be > 1kΩ (capacitor charging)
   - If < 100Ω, check for bridges

2. **Verify connections**:
   - Check continuity from each VDD pin to 3V3 net
   - Check continuity from each VSS pin to GND net

### Power-On Test

1. Connect 5V to J1 pin 5 (or via Arduino)
2. Connect GND to J1 pin 6
3. Measure voltage at U2 output: should be 3.3V ±5%
4. Measure voltage at any VDD pin: should be 3.3V
5. Check current draw: should be 40-60mA (idle with no firmware)

**Expected Power Consumption:**
- Idle mode: 40-60mA @ 3.3V (~0.15W)
- Active RF transmission: 150-200mA @ 3.3V (~0.5-0.7W)
- Ensure power supply can provide minimum 250mA @ 5V
- USB 2.0 port (500mA) or Arduino 5V pin is sufficient

### LED Test

With power applied:
- LED1 may be off (GPIO not configured)
- No smoke or excessive heat = good!

## Programming

### ICSP Connection

Connect PICkit or ICD to J3 (3-pin minimal ICSP header):

| J3 Pin | PICkit Pin | Signal |
|--------|------------|--------|
| 1 | 1 | MCLR |
| 2 | 4 | PGD |
| 3 | 5 | PGC |

**Note**: GND and VDD must be connected separately from the main power supply (J1 pins 5-6 or external 5V source). Ensure the board is powered before programming.

### First Program

1. Open MPLAB X IDE
2. Create new project for PIC32MX795F512L
3. Write simple LED blink test
4. Build and program
5. Verify LED1 blinks

## Troubleshooting

### No Power (3.3V not present)

- Check U2 orientation
- Check C1, C2 for shorts
- Verify 5V input present
- Check for solder bridges on U2

### PIC32 Not Responding

- Verify all VDD pins connected
- Check VCAP capacitor (C11) present
- Verify crystal connections
- Check MCLR pull-up (R1)
- Inspect for bridges on U1

### SPI Not Working

- Check series resistors (R7, R8)
- Verify pin mapping matches code
- Check for bridges on SPI pins
- Test with logic analyzer

### RF Section Issues

- Verify tuning capacitors installed
- Check antenna coil connection
- Measure resonant frequency with oscilloscope
- Adjust C12-C14 values if needed

## Final Testing

Once programmed with firmware:

1. Connect Arduino Nano via J1, J2
2. Power system
3. Run self-test routine
4. Verify SPI communication
5. Test RF output with oscilloscope
6. Test with actual Paxton reader (if available)

## Safety Notes

- Always disconnect power before making changes
- Use ESD precautions when handling ICs
- Don't exceed 5.5V input voltage
- Keep antenna away from sensitive electronics during testing
