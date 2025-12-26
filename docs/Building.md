# Hi-Tag 2 Emulator - Building and Programming Guide

## Overview

This guide covers the firmware build process for the Hi-Tag 2 Emulator, including toolchain setup, compilation, and programming the PIC32MX795F512L microcontroller.

## Prerequisites

### Required Software

1. **MPLAB X IDE** (v6.00 or later)
   - Download from: https://www.microchip.com/mplab/mplab-x-ide
   - Supported platforms: Windows, macOS, Linux
   - Free registration required

2. **XC32 Compiler** (v4.00 or later)
   - Download from: https://www.microchip.com/mplab/compilers
   - Free version is sufficient for this project
   - Must be installed before MPLAB X can compile PIC32 projects

3. **PICkit 3/4 or ICD 4** (for programming)
   - USB-based in-circuit programmer/debugger
   - Alternatives: Snap, ICD 5, or compatible third-party programmers

### Hardware Requirements

- Assembled Hi-Tag 2 Emulator PCB
- 5V power supply (USB or Arduino Nano)
- PICkit/ICD programmer
- 3 jumper wires (for ICSP connection)

## Toolchain Installation

### Windows

1. Download and install MPLAB X IDE
2. Download and install XC32 Compiler
3. Restart your computer
4. Connect your PICkit/ICD programmer

### macOS

```bash
# Install MPLAB X IDE (follow GUI installer)
# Install XC32 Compiler (follow GUI installer)

# Verify installation
/Applications/microchip/xc32/v4.00/bin/xc32-gcc --version
```

### Linux (Ubuntu/Debian)

```bash
# Install MPLAB X IDE
wget https://www.microchip.com/mplabx-ide-linux-installer.tar
tar -xf mplabx-ide-linux-installer.tar
sudo ./MPLABX-v6.00-linux-installer.sh

# Install XC32 Compiler
wget https://www.microchip.com/mplabxc32-linux-installer.run
chmod +x mplabxc32-linux-installer.run
sudo ./mplabxc32-linux-installer.run

# Add to PATH (add to ~/.bashrc)
export PATH=/opt/microchip/xc32/v4.00/bin:$PATH
```

## Project Setup

### Creating a New Project

1. Open MPLAB X IDE
2. File → New Project → Microchip Embedded → Standalone Project
3. Select Device: **PIC32MX795F512L**
4. Select Tool: Your PICkit/ICD programmer
5. Select Compiler: **XC32** (verify version is v4.00+)
6. Project Name: `HiTag2_Emulator_Firmware`
7. Click Finish

### Project Configuration

#### Device Configuration Bits

Add to your configuration header or main file:

```c
// PIC32MX795F512L Configuration Bit Settings

#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)
#pragma config FNOSC = PRIPLL           // Oscillator Selection (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (HS osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config DEBUG = ON               // Background Debugger Enable (Debugger is enabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)
```

**Configuration Explanation:**
- 8 MHz crystal × 20 (PLL) ÷ 2 = 80 MHz system clock
- Peripheral bus at 80 MHz (DIV_1)
- Watchdog timer disabled for development
- Debug mode enabled
- ICSP on PGC2/PGD2 (matches hardware design)

## Building the Firmware

### Command Line Build

```bash
# Navigate to project directory
cd HiTag2_Emulator_Firmware

# Clean previous build
make clean

# Build project
make

# Output files will be in dist/default/production/
# Main output: HiTag2_Emulator_Firmware.production.hex
```

### MPLAB X IDE Build

1. Open project in MPLAB X IDE
2. Select Project → Clean and Build
3. Verify build completes without errors
4. Check Output window for binary size information

### Build Configuration

For production builds:

1. Right-click project → Properties
2. XC32 Compiler → Optimizations
   - Optimization Level: `-O1` (balanced) or `-O2` (speed)
3. XC32 Linker → General
   - Verify heap size: 0 (not needed for embedded)
   - Verify stack size: appropriate for your application

## Programming the Device

### Hardware Setup

1. **Power the board**: Connect 5V to J1 pin 5, GND to J1 pin 6 (or power via Arduino)
2. **Connect ICSP**: Connect PICkit/ICD to J3 header

| PICkit Pin | J3 Pin | Signal |
|------------|--------|--------|
| 1 (MCLR) | 1 | MCLR/VPP |
| 2 (VDD) | - | Not connected* |
| 3 (GND) | - | Connect to board GND** |
| 4 (PGD) | 2 | Program Data |
| 5 (PGC) | 3 | Program Clock |
| 6 (NC) | - | Not connected |

*J3 is a 3-pin minimal header. VDD must come from main power supply.
**Connect PICkit GND to J1 pin 6 or J2 pin 4.

3. **Verify connections**: 
   - Board power LED should be on
   - PICkit/ICD LED should indicate connection

### Programming via MPLAB X IDE

1. Open project in MPLAB X IDE
2. Ensure correct device and programmer are selected
3. Click **Make and Program Device** (hammer + green arrow icon)
4. Progress bar will show:
   - Programming Flash... (30-60 seconds)
   - Verifying... (15-30 seconds)
5. Success message: "Programming/Verify complete"

### Programming via IPE (MPLAB Integrated Programming Environment)

For production programming without IDE:

1. Open MPLAB IPE
2. Select Device: PIC32MX795F512L
3. Select Tool: Your programmer
4. Click Connect
5. Browse to .hex file
6. Click Program
7. Wait for completion
8. Click Verify

### Command Line Programming (Advanced)

```bash
# Using pk2cmd (PICkit 2) or compatible tool
pk2cmd -PPIC32MX795F512L -F firmware.hex -M -R

# Using OpenOCD (with compatible programmer)
openocd -f interface/pickit2.cfg -f target/pic32mx.cfg \
  -c "program firmware.hex verify reset exit"
```

## Verification and Testing

### Post-Programming Checks

1. **Device ID Verification**: MPLAB X should report correct device ID (0x0938053)
2. **Configuration Bits**: Verify settings match your config
3. **Program Memory**: Should show programmed regions

### First Boot Test

```c
// Simple LED blink test firmware
#include <xc.h>
#include <sys/attribs.h>

#define LED1_PIN LATBbits.LATB10
#define LED1_TRIS TRISBbits.TRISB10

int main(void) {
    // Configure LED1 as output
    LED1_TRIS = 0;
    
    // Infinite loop
    while(1) {
        LED1_PIN = 1;  // LED on
        __delay_ms(500);
        LED1_PIN = 0;  // LED off
        __delay_ms(500);
    }
    
    return 0;
}
```

If LED1 blinks at 1 Hz, the system is working correctly.

## Troubleshooting

### Common Build Issues

#### Error: "xc32-gcc: command not found"
**Solution**: XC32 compiler not in PATH
```bash
# Add to ~/.bashrc or ~/.zshrc
export PATH=/opt/microchip/xc32/v4.00/bin:$PATH
```

#### Error: "Cannot find device PIC32MX795F512L"
**Solution**: Device pack not installed
- Tools → Packs → Search for "PIC32MX" → Install

#### Error: "Optimization level requires PRO license"
**Solution**: Using PRO-only optimization levels
- Change to `-O1` or `-O0` (free compiler)

### Common Programming Issues

#### "Target device not found"
**Checks**:
1. Is board powered? (Check 3.3V at VDD pins)
2. ICSP connections correct?
3. MCLR pull-up (R1) installed?
4. Try lower programming speed in MPLAB X settings

#### "Device ID incorrect"
**Solution**: Wrong device selected or faulty MCU
- Verify PIC32MX795F512L is selected
- Check U1 soldering under magnification

#### "Verification failed"
**Solutions**:
1. Power supply stable? (Check with scope)
2. Clean and retry programming
3. Erase device first: Device → Erase Device Memory

#### "Cannot enter programming mode"
**Checks**:
1. VCAP capacitor (C11) installed?
2. Crystal oscillating? (Check with scope on OSC2)
3. All VDD pins connected to 3.3V?
4. Try external reset (short MCLR to GND, then release)

### Debugging with MPLAB X

1. Set breakpoints in code
2. Click **Debug Main Project** (bug icon)
3. Use Debug → Step Over/Into for line-by-line execution
4. Watch variables in Variables window
5. Monitor SFRs in SFR window (Window → Debugging → SFRs)

## Firmware Architecture

### Recommended Project Structure

```
firmware/
├── src/
│   ├── main.c              # Main application
│   ├── hitag2.c            # Hi-Tag 2 protocol implementation
│   ├── rf_driver.c         # RF output control
│   ├── spi_slave.c         # SPI communication with Arduino
│   └── timer.c             # Precision timing for RF
├── include/
│   ├── hitag2.h
│   ├── rf_driver.h
│   ├── spi_slave.h
│   └── timer.h
├── config/
│   └── config_bits.h       # Configuration bits
└── Makefile
```

### Key Peripheral Initialization

```c
// SPI Slave Configuration (SPI1)
void SPI_Init(void) {
    SPI1CON = 0;                // Clear control register
    SPI1BRG = 0;                // Slave mode ignores baud rate
    SPI1STATbits.SPIROV = 0;    // Clear overflow flag
    SPI1CONbits.MSTEN = 0;      // Slave mode
    SPI1CONbits.CKE = 1;        // Data changes on idle-to-active
    SPI1CONbits.CKP = 0;        // Idle state low
    SPI1CONbits.MODE16 = 0;     // 8-bit mode
    SPI1CONbits.ON = 1;         // Enable SPI
}

// Timer for 125 kHz carrier (Timer1)
void Timer_Init_125kHz(void) {
    T1CON = 0;                  // Clear control register
    TMR1 = 0;                   // Clear timer value
    PR1 = 320;                  // 80 MHz / 320 / 2 = 125 kHz
    T1CONbits.TCKPS = 0;        // Prescaler 1:1
    IPC1bits.T1IP = 5;          // Interrupt priority 5
    IFS0bits.T1IF = 0;          // Clear interrupt flag
    IEC0bits.T1IE = 1;          // Enable interrupt
    T1CONbits.ON = 1;           // Enable timer
}

// GPIO for RF output (RB5)
void GPIO_Init(void) {
    TRISBbits.TRISB5 = 0;       // RF_OUT as output
    TRISBbits.TRISB4 = 1;       // RF_IN as input
    TRISBbits.TRISB10 = 0;      // LED1 as output
    TRISBbits.TRISB11 = 0;      // LED2 as output
}
```

## Additional Resources

### Documentation
- [PIC32MX Family Datasheet](https://www.microchip.com/wwwproducts/en/PIC32MX795F512L)
- [XC32 Compiler User Guide](https://www.microchip.com/XC32)
- [MPLAB X IDE User Guide](https://www.microchip.com/MPLABX)

### Example Code
- [Microchip Code Examples](https://github.com/microchip-pic-avr-examples)
- [PIC32 Peripheral Libraries](https://www.microchip.com/peripheral-libraries)

### Community Support
- [Microchip Forums](https://www.microchip.com/forums)
- [Stack Overflow - PIC32 Tag](https://stackoverflow.com/questions/tagged/pic32)

## License

Firmware is licensed under MIT License - see main repository LICENSE file.
