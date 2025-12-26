# Hi-Tag 2 Emulator - PIC32MX795F512L Pin Mapping

## Complete Pin Assignment Table

### Power Pins

| Pin | Function | Connection |
|-----|----------|------------|
| 2 | VDD | +3.3V |
| 16 | VDD | +3.3V |
| 30 | VDD | +3.3V |
| 37 | VDD | +3.3V |
| 46 | VDD | +3.3V |
| 55 | VDD | +3.3V |
| 62 | VDD | +3.3V |
| 86 | VDD | +3.3V |
| 15 | VSS | GND |
| 31 | VSS | GND |
| 36 | VSS | GND |
| 45 | VSS | GND |
| 65 | VSS | GND |
| 75 | VSS | GND |
| 85 | VCAP | 10µF to GND |

### Crystal Oscillator

| Pin | Function | Connection |
|-----|----------|------------|
| 64 | OSC1/CLKI | 8MHz Crystal + 18pF to GND |
| 63 | OSC2/CLKO | 8MHz Crystal + 18pF to GND |

### Reset

| Pin | Function | Connection |
|-----|----------|------------|
| 13 | MCLR | 10kΩ pull-up to 3.3V, J2 pin 1, J3 pin 1 |

### SPI Interface (to Arduino)

| Pin | Function | Net Name | Connection |
|-----|----------|----------|------------|
| 25 | SDI1 | SPI_MOSI | J1 pin 1 via 100Ω (R7) |
| 72 | SDO1 | SPI_MISO | J1 pin 2 (direct) |
| 70 | SCK1 | SPI_SCK | J1 pin 3 via 100Ω (R8) |
| 69 | SS1 | SPI_CS | J1 pin 4 (direct) |

### Control Signals

| Pin | Function | Net Name | Connection |
|-----|----------|----------|------------|
| 17 | RA0 | TX_EN | J2 pin 2 (Transmit Enable) |
| 38 | RA1 | IRQ | J2 pin 3 (Interrupt Request) |

### RF Interface

| Pin | Function | Net Name | Connection |
|-----|----------|----------|------------|
| 18 | RB5 | RF_OUT | R6 (1MΩ) → Tuning caps → Antenna |
| 12 | RB4 | RF_IN | U4 Schmitt trigger output (COIL_SENSE) |

### Status LEDs

| Pin | Function | Net Name | Connection |
|-----|----------|----------|------------|
| 23 | RB10 | STATUS1 | R2 (330Ω) → LED1 (Green) → GND |
| 24 | RB11 | STATUS2 | R3 (330Ω) → LED2 (Red) → GND |

### ICSP Programming

| Pin | Function | Connection |
|-----|----------|------------|
| 13 | MCLR | J3 pin 1 |
| 27 | PGED2 | J3 pin 2 (PGD) |
| 26 | PGEC2 | J3 pin 3 (PGC) |

## Connector Pinouts

### J1 - SPI Arduino Interface (1x6 Header)

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1 | MOSI | Arduino → PIC32 | Via 100Ω series resistor |
| 2 | MISO | PIC32 → Arduino | Direct connection |
| 3 | SCK | Arduino → PIC32 | Via 100Ω series resistor |
| 4 | CS | Arduino → PIC32 | Active low chip select |
| 5 | 5V | Power | Input from Arduino |
| 6 | GND | Ground | Common ground |

### J2 - Control Arduino Interface (1x4 Header)

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1 | RESET | Arduino → PIC32 | Active low reset (MCLR) |
| 2 | TX_EN | Arduino → PIC32 | Transmit enable |
| 3 | IRQ | PIC32 → Arduino | Interrupt request |
| 4 | GND | Ground | Common ground |

### J3 - ICSP Programming Header (1x3 Header)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | MCLR | Master Clear / VPP |
| 2 | PGD | Program Data (PGED2) |
| 3 | PGC | Program Clock (PGEC2) |

**Note**: This is a minimal 3-pin ICSP header. GND and VDD must be supplied from the main power supply (connect J1 to Arduino or external 5V source) before programming.

### J4 - Antenna Connector (1x2 Header)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | COIL_A | Antenna coil connection |
| 2 | GND | Ground return |

## Arduino Nano Pin Mapping

For reference, here's how the Arduino Nano connects to the Hi-Tag 2 Emulator:

| Arduino Pin | Function | Connects To |
|-------------|----------|-------------|
| D0 (RX) | UART RX | Flipper Zero TX |
| D1 (TX) | UART TX | Flipper Zero RX |
| D2 | Digital Out | J2 pin 1 (RESET) |
| D3 | Digital Out | J2 pin 2 (TX_EN) |
| D4 | Digital In | J2 pin 3 (IRQ) |
| D10 | SPI SS | J1 pin 4 (CS) |
| D11 | SPI MOSI | J1 pin 1 (MOSI) |
| D12 | SPI MISO | J1 pin 2 (MISO) |
| D13 | SPI SCK | J1 pin 3 (SCK) |
| 5V | Power | J1 pin 5 |
| GND | Ground | J1 pin 6, J2 pin 4 |

## Flipper Zero GPIO Mapping

| Flipper Pin | Function | Connects To |
|-------------|----------|-------------|
| TX | UART TX | Arduino D0 (RX) |
| RX | UART RX | Arduino D1 (TX) |
| GND | Ground | Arduino GND |

## Notes

1. **All VDD pins must be connected** - The PIC32 requires all VDD pins to be connected for proper operation.

2. **VCAP is mandatory** - Pin 85 must have a 10µF capacitor to ground for the internal voltage regulator.

3. **Series resistors on SPI** - The 100Ω resistors on MOSI and SCK help with signal integrity and protect against shorts during development.

4. **MCLR pull-up** - The 10kΩ pull-up on MCLR prevents accidental resets from noise.

5. **Crystal placement** - Keep the crystal and load capacitors as close to pins 63/64 as possible.
