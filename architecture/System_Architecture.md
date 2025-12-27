# System Architecture: HiTag 2 PCB Emulator

## 1. Introduction
The HiTag 2 PCB Emulator is a high-performance 125 kHz RFID research tool designed for full-page spoofing (Pages 0-7) of HiTag 2 transponders. The system employs a three-tier architecture to balance user experience, logical control, and real-time RF performance.

## 2. Three-Tier Architecture

### Tier 1: Flipper Zero (User Interface & Command Input)
The Flipper Zero serves as the primary interaction point for the user.
- **Functionality**:
    - Provides a graphical user interface (GUI) for selecting UIDs, Secret Keys, and operating modes.
    - Captures and displays real-time logs from the emulator.
    - Stores "dictionaries" of keys and UID files on its SD card.
- **Interface**: Connects to the Arduino Nano 33 BLE Rev2 via a serial UART link (typically via the GPIO header).
- **Control Flow**: User selects an action (e.g., "Emulate Paxton Token") -> Flipper sends high-level parameters over UART to the Arduino.

### Tier 2: Arduino Nano 33 BLE Rev2 (Main Controller)
The Arduino Nano 33 BLE Rev2, powered by the Nordic nRF52840 SoC, acts as the "brain" of the operation.
- **Functionality**:
    - **Logic Orchestrator**: Receives commands from the Flipper Zero and coordinates the emulation process.
    - **Protocol Handling**: Manages high-level HiTag 2 logic, such as switching between Public, Password, and Crypto modes.
    - **Data Management**: Can store temporary buffers of page data and cryptographic states.
    - **Wireless Connectivity**: The nRF52840 provides Bluetooth Low Energy (BLE) capabilities, allowing for future smartphone-based control.
- **Interface**:
    - **Downlink**: UART (to Flipper Zero).
    - **Uplink**: SPI (to PIC32MX795F512L).
- **Role**: It translates the user's intent into low-level instructions for the RF engine.

### Tier 3: PIC32MX795F512L (125 kHz RF Engine)
The PIC32MX795F512L is the core RF modulation/demodulation engine.
- **Functionality**:
    - **Real-Time RF Processing**: Handles the 125 kHz carrier interaction with nanosecond precision.
    - **Modulation**: Implements Load Modulation (Manchester/Bi-Phase) for the Uplink (Tag to Reader).
    - **Demodulation**: Decodes 100% ASK signals from the Reader (Downlink) using its Input Capture modules or high-speed ADC.
    - **Crypto Implementation**: Runs the 48-bit HITAG 2 stream cipher in real-time to respond to authentication challenges within the required 256 µs window.
    - **Bit-Banging**: Precise timing of the 4 kbps / 8 µs bit periods.
- **Hardware Features utilized**:
    - **80 MHz Clock**: Provides 12.5 ns instruction cycles, essential for stable 125 kHz timing.
    - **SPI Slave**: Receives emulation data (UID, Keys, Page Content) from the Arduino.
    - **Timers/Output Compare**: Generates the subcarrier and handles pulse width measurement for decoding.
- **Analog Front-End**: The PIC32 drives an external LC tank circuit tuned to 125 kHz, with a MOSFET used for load modulation.

## 3. Communication Flow

1. **Configuration Phase**:
   - Flipper Zero sends the target UID and Secret Key to the Arduino Nano via UART.
   - Arduino Nano pushes this configuration to the PIC32 via SPI.
   - PIC32 initializes its internal HiTag 2 state machine and waits for an RF field.

2. **Detection Phase**:
   - The PIC32 detects a 125 kHz carrier from a reader.
   - It signals the "Field Detected" state back to the Arduino (via a dedicated GPIO or SPI status).
   - Arduino may inform the Flipper Zero to update the UI.

3. **Emulation Phase**:
   - The Reader sends a `START_AUTH` command.
   - PIC32's RF frontend captures the ASK signal; the CPU decodes it in real-time.
   - PIC32 computes the cryptographic response using the 48-bit cipher.
   - PIC32 modulates the antenna load to send the UID and Auth Response back to the reader.
   - Subsequent `READ_PAGE` commands are handled entirely by the PIC32 to ensure timing compliance.

## 4. Why This Architecture?
- **Timing Accuracy**: Using a dedicated MCU (PIC32) for RF ensures that the strict timing requirements of HiTag 2 (especially the 256 µs response window) are never interrupted by UI tasks or Bluetooth interrupts.
- **Flexibility**: The Arduino/nRF52840 layer allows for easy integration of modern features like BLE or complex logic without touching the performance-critical RF code.
- **User Experience**: Leveraging the Flipper Zero provides a ready-made, portable interface with a screen and buttons, making the tool accessible in the field.

## 5. Emulation Capabilities
- **Full Page Spoofing**: Ability to emulate all 8 pages (256 bits) of HITAG 2 memory.
- **Crypto Mode Support**: Real-time 48-bit stream cipher execution.
- **Paxton NET2 Compatibility**: Specifically tuned for the timing and protocol quirks of Paxton access control readers.
- **Multi-Token Storage**: Flipper Zero can store hundreds of token profiles, switchable on-the-fly.
