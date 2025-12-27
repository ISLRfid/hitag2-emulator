# Hi-Tag 2 Emulator - Troubleshooting Guide

This guide covers common issues and their solutions when installing and operating the Hi-Tag 2 emulator system.

---

## Table of Contents

1. [PIC32 Issues](#pic32-issues)
2. [Arduino Issues](#arduino-issues)
3. [Flipper Zero App Issues](#flipper-zero-app-issues)
4. [Communication Issues](#communication-issues)
5. [RF/Protocol Issues](#rfprotocol-issues)
6. [Hardware Issues](#hardware-issues)
7. [Diagnostic Commands](#diagnostic-commands)

---

## PIC32 Issues

### Symptom: No response on SPI

**Possible Causes:**
- PIC32 not programmed correctly
- Power not reaching PIC32
- SPI pins not connected correctly
- Crystal not oscillating

**Diagnosis:**
1. Check Status LED (RB10) - should be ON
2. Check RF LED (RB11) - may be OFF if no RF field
3. Measure 3.3V on VDD pins

**Solutions:**
```bash
# 1. Verify hex file was created
ls -la firmware/pic32/hitag2_emulator.hex

# 2. Reprogram PIC32
cd firmware/pic32
make clean
make
pic32prog -d /dev/ttyUSB0 -b 115200 hitag2_emulator.hex

# 3. Check crystal oscillator with oscilloscope
# Should see 8 MHz sine wave on OSC1/OSC2 pins
```

### Symptom: Carrier not generating (125 kHz)

**Possible Causes:**
- PWM pin (RB5) not configured
- Timer not running
- TX enable not asserted

**Diagnosis:**
1. Check PWM output on RB5 with oscilloscope
2. Verify Timer3 is running
3. Check TX_EN signal (RA0)

**Solutions:**
```c
// In rf_driver.c, verify timer configuration
void rf_driver_init(void) {
    // PWM on RB5 (Timer3 OC1)
    T3CONbits.TCKPS = 0b001;  // 1:2 prescaler
    PR3 = 640 - 1;  // 80 MHz / 2 / 640 = 62.5 kHz (for 125 kHz carrier)
    
    // OC1 for PWM
    OC1CONbits.OCM = 0b110;  // PWM mode
    OC1RS = 320;  // 50% duty cycle
}

// TX enable should be set by Arduino before transmission
digitalWrite(PIN_TX_EN, HIGH);  // Arduino side
```

### Symptom: Memory errors / Random resets

**Possible Causes:**
- Crystal oscillator out of specification
- Power supply instability
- Improper decoupling capacitors

**Solutions:**
1. Verify crystal is 8 MHz ±20 ppm
2. Check 3.3V supply with oscilloscope (look for ripple)
3. Add 100nF decoupling caps near VDD pins
4. Check MCLR pull-up resistor (10kΩ recommended)

### Symptom: Won't enter programming mode

**Solutions:**
1. Hold MCLR low during power-on
2. Check PGC/PGD connections to programmer
3. Verify VDD is present during programming
4. Try different programming speed:
   ```bash
   pic32prog -d /dev/ttyUSB0 -b 115200 hitag2_emulator.hex
   # Try lower baud rate if fails
   pic32prog -d /dev/ttyUSB0 -b 9600 hitag2_emulator.hex
   ```

---

## Arduino Issues

### Symptom: Serial not responding

**Diagnosis:**
1. Check USB connection
2. Verify correct COM port selected
3. Check Serial Monitor baud rate (115200)

**Solutions:**
```cpp
// Add debug output in setup()
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(100);  // Wait for Serial Monitor
    }
    Serial.println(F("DEBUG: setup() entered"));
    // ... rest of initialization
}

// Check if Arduino is recognized
ls -la /dev/ttyUSB*
# Should show /dev/ttyUSB0 or similar when Arduino connected

# Check USB device
lsusb
# Should show Arduino Nano 33 BLE
```

### Symptom: SPI communication errors

**Possible Causes:**
- Wrong SPI mode
- Pin connections incorrect
- PIC32 not ready

**Solutions:**
```cpp
// Verify SPI initialization
void setup() {
    SPI.begin();
    // Mode 0: CPOL=0, CPHA=0
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    
    pinMode(PIN_SPI_SS, OUTPUT);
    digitalWrite(PIN_SPI_SS, HIGH);  // Deselect PIC32
}

// Check pin connections with multimeter
// Arduino D10 should connect to PIC32 pin 69
// Arduino D11 should connect to PIC32 pin 25
// Arduino D12 should connect to PIC32 pin 72
// Arduino D13 should connect to PIC32 pin 70
```

### Symptom: Token not loading

**Diagnosis:**
```python
# Test with Python script
import serial

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# Ping
ser.write(b'\x02\x01\x00\x03\n')
response = ser.readline()
print(f"Response: {response.hex()}")

# Should see response like: 02 01 00 <checksum>
```

**Solutions:**
1. Check selected token index is valid
2. Verify token UID is not zero
3. Check for token storage overflow (max 10 tokens)

### Symptom: Arduino LED not blinking during emulation

**Diagnosis:**
1. Check LED pin 13 connection
2. Verify emulation_active flag is set

**Solutions:**
```cpp
// Add debug in loop()
void loop() {
    // Process commands
    if (uart_frame_complete) {
        process_uart_command();
    }
    
    // Debug: print emulation status
    if (millis() % 1000 < 10) {
        Serial.print(F("Emulation active: "));
        Serial.println(emulation_active);
    }
    
    // Update status LED
    update_status();
}

void update_status() {
    if (emulation_active) {
        // Blink at 2Hz
        digitalWrite(STATUS_LED, (millis() / 500) % 2);
    } else {
        digitalWrite(STATUS_LED, HIGH);
    }
}
```

---

## Flipper Zero App Issues

### Symptom: App not appearing in menu

**Solutions:**
1. **Rebuild with correct SDK:**
   ```bash
   cd firmware/flipper
   ufbt clean
   ufbt build
   ```

2. **Verify FAP file:**
   ```bash
   # Check if FAP was created
   ls -la firmware/flipper/*.fap
   
   # Should show:
   # hitag2_emulator.fap
   ```

3. **Manual installation:**
   - Copy FAP to Flipper via qFlipper
   - Location: `/apps/RFID/`

### Symptom: Connection failed

**Diagnosis:**
1. Check USB cable is data-capable
2. Verify Arduino is powered
3. Check UART connection

**Solutions:**
```c
// In hitag2_serial.c, verify UART config
void serial_alloc(void) {
    // Configure UART at 115200
    furi_hal_uart_init(FuriHalUartIdUSART1, 115200);
}

// Add connection test
bool serial_ping(SerialConnection* serial) {
    uint8_t response[8];
    uint8_t response_len = sizeof(response);
    
    return serial_send_command(serial, CMD_PING, NULL, 0, response, &response_len);
}
```

### Symptom: Display issues / Garbled text

**Solutions:**
1. Check display orientation
2. Verify font configuration
3. Rebuild with latest SDK:
   ```bash
   ufbt update
   ufbt build
   ```

### Symptom: App crashes on launch

**Diagnosis:**
1. Check memory allocation
2. Verify view allocation order

**Solutions:**
```c
// In hitag2_app.c, add safety checks
App* app_alloc(void) {
    App* app = malloc(sizeof(App));
    if (!app) return NULL;
    
    memset(app, 0, sizeof(App));
    
    // Allocate views in order
    app->view_dispatcher = view_dispatcher_alloc();
    if (!app->view_dispatcher) goto error;
    
    app->scene_manager = scene_manager_alloc(&scene_handlers, app);
    if (!app->scene_manager) goto error;
    
    // ... rest of allocation
    
    return app;

error:
    // Cleanup on error
    app_free(app);
    return NULL;
}
```

---

## Communication Issues

### Symptom: UART timeout errors

**Possible Causes:**
- Incorrect baud rate
- Flow control issues
- Buffer overflow

**Solutions:**
```bash
# Verify baud rate on both ends (should be 115200)
# Arduino: Serial.begin(115200)
# Flipper: furi_hal_uart_init(FuriHalUartIdUSART1, 115200)

# Check for framing errors
# Use logic analyzer or oscilloscope to verify signal levels

# Increase timeout in Flipper app (hitag2_serial.c)
uint32_t timeout = furi_get_tick() + 2000;  // 2 seconds instead of 1
```

### Symptom: Checksum errors

**Frame Format:**
```
┌────┬────┬────┬────────┬────────┐
│ SOF│ CMD│LEN │ DATA   │ CHECK  │
│ 0x02│    │    │ LEN B  │ SUM    │
└────┴────┴────┴────────┴────────┘

Checksum = SOF ^ CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[n]
```

**Diagnosis:**
```python
# Verify checksum calculation
def calc_checksum(cmd, data):
    checksum = 0x02 ^ cmd ^ len(data)
    for b in data:
        checksum ^= b
    return checksum

# Test
cmd = 0x01
data = []
checksum = calc_checksum(cmd, data)
print(f"Expected checksum: {checksum:02X}")
```

### Symptom: Commands not executing

**Solutions:**
1. Verify command format
2. Check command table in Arduino sketch
3. Add command logging:
   ```cpp
   void process_uart_command() {
       Serial.print(F("Received cmd: 0x"));
       Serial.println(uart_rx_buffer[1], HEX);
       Serial.print(F("Length: "));
       Serial.println(uart_rx_index);
       // ... rest of processing
   }
   ```

---

## RF/Protocol Issues

### Symptom: Reader not detecting emulation

**Diagnosis:**
1. Check antenna coil connection
2. Verify 125 kHz carrier generation
3. Check modulation signal

**Solutions:**
```c
// In rf_driver.c, verify carrier generation
void rf_carrier_on(void) {
    // Enable PWM on RB5
    OC1CONbits.ON = 1;
    
    // Enable TX driver
    LATAbits.LATA0 = 1;  // TX_EN high
}

void rf_carrier_off(void) {
    // Disable PWM
    OC1CONbits.ON = 0;
    
    // Disable TX driver
    LATAbits.LATA0 = 0;
}

// Check with oscilloscope
// RB5 should show 125 kHz square wave during transmission
```

### Symptom: Authentication failures

**Possible Causes:**
- Incorrect authentication key
- Clock drift between systems
- Manchester encoding errors

**Solutions:**
```c
// In crypto.c, verify LFSR implementation
void crypto_init(void) {
    // LFSR for 48-bit stream cipher
    g_lfsr_state = 0x123456789ABC;
}

uint32_t crypto_lfsr(uint32_t key, uint32_t input) {
    uint32_t state = key;
    uint32_t output = 0;
    
    for (int i = 0; i < 32; i++) {
        // LFSR next state
        uint32_t feedback = ((state >> 31) ^ ((state >> 21) & 1)) & 1;
        state = (state << 1) | feedback;
        
        // XOR with input
        output = (output << 1) | ((state ^ input) & 1);
    }
    
    return output;
}
```

### Symptom: Read/Write page errors

**Solutions:**
```c
// In memory.c, verify page operations
uint8_t memory_read_page(uint8_t page, uint32_t* data) {
    if (page > 7) return ERR_INVALID_PAGE;
    
    *data = g_tag_memory[page];
    return STATUS_OK;
}

uint8_t memory_write_page(uint8_t page, uint32_t data) {
    if (page > 7) return ERR_INVALID_PAGE;
    if (page == 0) return ERR_READ_ONLY;  // UID is read-only
    
    g_tag_memory[page] = data;
    return STATUS_OK;
}
```

---

## Hardware Issues

### Symptom: PIC32 getting hot

**Immediate Actions:**
1. Disconnect power immediately
2. Check for shorts on PCB
3. Verify correct component orientation
4. Check voltage on VDD pins (should be 3.3V, not 5V)

### Symptom: Intermittent operation

**Possible Causes:**
- Cold solder joints
- Loose connections
- Power supply droop
- EMI interference

**Solutions:**
1. Re-solder all connections
2. Use shorter USB cables
3. Add ferrite beads on power lines
4. Separate high-current lines from signal lines

### Symptom: No 3.3V output

**Diagnosis:**
1. Check AMS1117-3.3 regulator input (should be ~5V)
2. Measure output under load
3. Check decoupling capacitors

**Solutions:**
```text
Power Supply Circuit:
┌─────────────┐
│ USB 5V      │
└──────┬──────┘
       │
    ┌──┴──┐
    │ C1  │ 100uF
    └──┬──┘
       │
┌──────▼──────┐
│ AMS1117-3.3 │
│    LDO      │
└──────┬──────┘
       │
    ┌──┴──┐
    │ C2  │ 100nF
    └──┬──┘
       │
    ┌──▼──┐
    │3.3V │ → PIC32 VDD
    └─────┘
```

---

## Diagnostic Commands

### Arduino Serial Test

```cpp
// Add to Arduino sketch for debugging
void debug_dump_state() {
    Serial.println(F("=== Debug State ==="));
    Serial.print(F("Selected token: "));
    Serial.println(selected_token_index);
    Serial.print(F("Emulation active: "));
    Serial.println(emulation_active);
    
    if (selected_token_index >= 0) {
        Serial.print(F("UID: 0x"));
        Serial.println(tokens[selected_token_index].uid, HEX);
    }
    
    Serial.print(F("IRQ pin: "));
    Serial.println(digitalRead(PIN_IRQ));
    
    Serial.print(F("TX_EN pin: "));
    Serial.println(digitalRead(PIN_TX_EN));
    
    Serial.print(F("SS pin: "));
    Serial.println(digitalRead(PIN_SPI_SS));
    
    Serial.println(F("=================="));
}
```

### Python Test Script

```python
#!/usr/bin/env python3
"""
Hi-Tag 2 Emulator - Diagnostic Test Script
"""

import serial
import time

class Hitag2Diag:
    def __init__(self, port='/dev/ttyUSB0'):
        self.ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for Arduino init
    
    def send_cmd(self, cmd, data=b''):
        """Send command and get response"""
        # Build frame
        sof = 0x02
        length = len(data)
        checksum = sof ^ cmd ^ length
        for b in data:
            checksum ^= b
        
        frame = bytes([sof, cmd, length]) + data + bytes([checksum, 0x0A])
        
        self.ser.write(frame)
        time.sleep(0.1)
        
        # Read response
        response = self.ser.readline()
        return response
    
    def ping(self):
        """Test connection"""
        response = self.send_cmd(0x01)
        print(f"Ping response: {response.hex()}")
        return len(response) > 0
    
    def get_status(self):
        """Get system status"""
        response = self.send_cmd(0x30)
        print(f"Status: {response.hex()}")
        return response
    
    def list_tokens(self):
        """List stored tokens"""
        response = self.send_cmd(0x12)
        print(f"Tokens: {response.hex()}")
        return response
    
    def select_token(self, slot):
        """Select token by slot (1-based)"""
        response = self.send_cmd(0x13, bytes([slot]))
        print(f"Select slot {slot}: {response.hex()}")
        return response
    
    def start_emulate(self):
        """Start emulation"""
        response = self.send_cmd(0x40)
        print(f"Start emulate: {response.hex()}")
        return response
    
    def stop_emulate(self):
        """Stop emulation"""
        response = self.send_cmd(0x41)
        print(f"Stop emulate: {response.hex()}")
        return response
    
    def close(self):
        self.ser.close()

if __name__ == "__main__":
    diag = Hitag2Diag()
    
    print("=== Hi-Tag 2 Diagnostic Test ===")
    
    # Test 1: Ping
    print("\n1. Testing connection...")
    if diag.ping():
        print("   [PASS] Arduino responding")
    else:
        print("   [FAIL] No response from Arduino")
    
    # Test 2: Status
    print("\n2. Getting status...")
    diag.get_status()
    
    # Test 3: List tokens
    print("\n3. Listing tokens...")
    diag.list_tokens()
    
    # Test 4: Start/stop emulation
    print("\n4. Testing emulation control...")
    diag.start_emulate()
    time.sleep(1)
    diag.stop_emulate()
    
    diag.close()
    print("\n=== Test Complete ===")
```

### Flipper Zero Debug View

The Flipper app includes a debug view that shows communication logs:

1. From main view, press **DOWN** arrow
2. Debug log displays:
   - UART commands sent
   - Responses received
   - Connection status
   - Error messages

---

## Contact for Additional Help

If issues persist after trying these solutions:

1. Check GitHub issues for similar problems
2. Review [Building.md](Building.md) for build-specific issues
3. Review [HiTag2_Protocol.md](HiTag2_Protocol.md) for protocol issues
4. Open a new issue with:
   - Detailed description of problem
   - Steps to reproduce
   - Hardware configuration
   - Diagnostic output

---

## Disclaimer

This troubleshooting guide is for educational and research purposes. Always ensure proper authorization when testing RFID systems.
