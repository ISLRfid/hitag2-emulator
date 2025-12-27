# Hi-Tag 2 Emulator - User Guide

This guide explains how to use the Hi-Tag 2 Emulator application on Flipper Zero.

## Table of Contents

1. [Introduction](#introduction)
2. [Main Screen](#main-screen)
3. [Token Management](#token-management)
4. [Emulation Control](#emulation-control)
5. [Debug View](#debug-view)
6. [Workflow Examples](#workflow-examples)
7. [Tips and Best Practices](#tips-and-best-practices)

---

## Introduction

The Hi-Tag 2 Emulator allows you to:

- **Store and manage** up to 10 RFID tokens
- **Emulate** Hi-Tag 2 transponders (125 kHz)
- **Authenticate** with Paxton NET2 access control systems
- **Monitor** communication in real-time

### System Architecture

```
┌─────────────┐     UART      ┌──────────────┐      SPI       ┌─────────────────┐
│ Flipper Zero│◄────────────►│ Arduino Nano │◄──────────────►│ PIC32MX795F512L │
│  (You)      │   115200     │  (Bridge)    │  4 MHz, Mode 0 │   (RF Engine)   │
└─────────────┘              └──────────────┘                └────────┬────────┘
                                                                       │
                                                               ┌───────▼───────┐
                                                               │  125 kHz RF   │
                                                               │  Antenna      │
                                                               └───────────────┘
```

---

## Main Screen

### Screen Layout

```
┌─────────────────────────────────────────┐
│ Hi-Tag 2 Emulator                       │  ← Title bar
│                                         │
│ Status: Ready           [○] Connected   │  ← Connection status
│                                         │
│ UID: DEADBEEF                          │  ← Current token UID
│ Token: #1                               │  ← Selected slot
│                                         │
│ [OK] Select    [R] Read    [↑] Start   │  ← Button hints
│ [↓] Debug      [←] Write              │
└─────────────────────────────────────────┘
```

### Status Indicators

| Status | Meaning |
|--------|---------|
| **Ready** | System ready, no emulation active |
| **ACTIVE** | Emulation running, RFID field detected |
| **[○] Connected** | Arduino bridge detected |
| **[●] Disconnected** | Arduino bridge not detected |

### Button Functions

| Button | Function | Description |
|--------|----------|-------------|
| **OK** | Select Token | Open token list view |
| **UP** | Toggle Emulation | Start/stop RFID emulation |
| **DOWN** | Debug View | Show communication log |
| **LEFT** | Write Token | Write token to Arduino |
| **RIGHT** | Read Token | Read token from Arduino |
| **BACK** | Exit | Close application |

### Connection Status

When the Arduino bridge is connected:
- Shows "Connected" in green
- All control functions available

When disconnected:
- Shows "Disconnected" in yellow
- Only basic UI functions work
- Check USB cable and Arduino power

---

## Token Management

### Token Structure

Each token contains:

```
┌─────────────────────────────────────┐
│ Token Structure                     │
├─────────────────────────────────────┤
│ UID (32-bit)     0xDEADBEEF         │  ← Unique identifier
│ Config (32-bit)  0x00000000         │  ← Configuration flags
│ Key (48-bit)     0x123456789ABC     │  ← Authentication key
│ User Data (128-bit)                 │  ← User storage
└─────────────────────────────────────┘
```

### Token List View

Press **OK** from main screen to open token list:

```
┌─────────────────────────────────────────┐
│ Select Token                            │
│                                         │
│ #1: DEADBEEF    ◄── Selected            │
│ #2: A1B2C3D4                          │
│ #3: 11223344                           │
│ #4: FFEEDDCC                           │
│ #5:                                     │
│                                         │
│ [OK] Select    [Back] Cancel            │
└─────────────────────────────────────────┘
```

### Selecting a Token

1. Press **OK** to open token list
2. Use **UP/DOWN** to navigate
3. Press **OK** to select
4. Token UID displays on main screen

### Adding a New Token

Tokens are added programmatically or via the Edit view.

**Using Default Token:**

The app creates a default token on first launch:
- UID: `0xDEADBEEF`
- Key: `0x123456789ABC`

**Creating Custom Token:**

1. From token list, select empty slot
2. Edit UID and key values
3. Save token

### Deleting a Token

1. Navigate to token in list
2. (Token deletion via API or debug commands)

---

## Emulation Control

### Starting Emulation

1. **Connect Arduino bridge** (verify "Connected" status)
2. **Select a token** (UID displayed)
3. **Press UP** to start

```
┌─────────────────────────────────────────┐
│ Hi-Tag 2 Emulator                       │
│                                         │
│ Status: ACTIVE                          │  ← Changed to ACTIVE
│ RFID field detected                     │
│                                         │
│ UID: DEADBEEF                           │
│ Token: #1                               │
│                                         │
│ [↑] Stop      [↓] Debug                 │
└─────────────────────────────────────────┘
```

### During Emulation

- **RF LED on Arduino** blinks (500ms interval)
- **Flipper shows** "Status: ACTIVE"
- **Emulator responds** to reader queries

### Stopping Emulation

Press **UP** again to stop:

```
┌─────────────────────────────────────────┐
│ Hi-Tag 2 Emulator                       │
│                                         │
│ Status: Ready                           │  ← Back to Ready
│                                         │
│ UID: DEADBEEF                           │
│ Token: #1                               │
│                                         │
│ [↑] Start     [↓] Debug                 │
└─────────────────────────────────────────┘
```

### Reading Token from Reader

Press **RIGHT** to read token from Arduino memory.

**Response:**
```
Reading token from Arduino...
Token loaded: UID=0xDEADBEEF
```

### Writing Token to Reader

Press **LEFT** to write current token to Arduino.

**Response:**
```
Writing token to Arduino...
Token saved to slot #1
```

---

## Debug View

Press **DOWN** from main screen to open debug view:

```
┌─────────────────────────────────────────┐
│ Debug Log                               │
│                                         │
│ Connected                               │  ← Connection status
│                                         │
│ TX: 02 10 00 12                         │  ← Commands sent
│ RX: 02 10 00 00                         │  ← Responses
│                                         │
│ TX: 02 40 00 46                         │
│ RX: 02 40 00 00                         │
│                                         │
│ [Back] Return                           │
└─────────────────────────────────────────┘
```

### Debug Log Format

| Prefix | Meaning |
|--------|---------|
| **TX** | Data sent to Arduino |
| **RX** | Data received from Arduino |
| **ERR** | Error occurred |
| **INFO** | Information message |

### Interpreting Log Entries

```
TX: 02 10 00 12
    │ │  │  └── Checksum
    │ │  └───── Data length (0 bytes)
    │ └──────── Command (0x10 = LOAD_TOKEN)
    └────────── SOF (0x02 = Start of Frame)
```

### Common Commands in Log

| Command | Hex | Description |
|---------|-----|-------------|
| PING | 0x01 | Test connection |
| RESET | 0x02 | Reset PIC32 |
| LOAD_TOKEN | 0x10 | Load token to PIC32 |
| SAVE_TOKEN | 0x11 | Save token from PIC32 |
| LIST_TOKENS | 0x12 | List stored tokens |
| SELECT_TOKEN | 0x13 | Select token slot |
| START_EMULATE | 0x40 | Start emulation |
| STOP_EMULATE | 0x41 | Stop emulation |

---

## Workflow Examples

### Example 1: Basic Emulation

**Goal:** Emulate a known token with UID `0x12345678`

1. **Launch App**
   - Navigate to `Apps → RFID → Hi-Tag 2 Emulator`
   - Press OK to launch

2. **Verify Connection**
   ```
   Status: Ready
   [○] Connected should appear
   ```

3. **Load Token**
   - Token already loaded with default UID
   - Or select from list with OK

4. **Start Emulation**
   - Press UP
   - Status changes to "ACTIVE"

5. **Test with Reader**
   - Hold emulator near reader
   - Reader should recognize token

6. **Stop Emulation**
   - Press UP again
   - Status returns to "Ready"

### Example 2: Using Multiple Tokens

**Goal:** Switch between different stored tokens

1. **Launch App**
   - Navigate to app
   - Token #1 selected by default

2. **Open Token List**
   - Press OK
   - Token list displays

3. **Select Different Token**
   ```
   #1: 12345678
   #2: 87654321  ◄ Select this
   #3: AABBCCDD
   ```
   - Press UP/DOWN to select
   - Press OK to confirm

4. **Verify Selection**
   - Return to main screen
   - UID displays `0x87654321`

5. **Emulate Selected Token**
   - Press UP to start
   - Reader sees token #2

### Example 3: Debugging Communication

**Goal:** View UART communication for troubleshooting

1. **Launch App**
   - Open Hi-Tag 2 Emulator

2. **Open Debug View**
   - Press DOWN

3. **Monitor Traffic**
   ```
   TX: 02 01 00 03    ← PING command
   RX: 02 01 00 00    ← PONG response
   
   TX: 02 12 00 16    ← LIST_TOKENS
   RX: 02 12 05 01... ← Token list response
   ```

4. **Check for Errors**
   - Look for "ERR" entries
   - Verify TX/RX pairs match

5. **Return to Normal View**
   - Press BACK

### Example 4: Reading from Arduino Memory

**Goal:** Read token currently stored in Arduino

1. **Launch App**
   - Open Hi-Tag 2 Emulator

2. **Read Token**
   - Press RIGHT
   - App reads from Arduino

3. **Verify Result**
   ```
   Reading token from Arduino...
   Token loaded: UID=0xDEADBEEF
   ```

4. **Token Now Active**
   - UID displayed on main screen
   - Ready for emulation

### Example 5: Writing to Arduino Memory

**Goal:** Save current token to Arduino memory

1. **Configure Token**
   - Select desired token
   - Verify UID

2. **Write Token**
   - Press LEFT
   - App writes to Arduino

3. **Verify Result**
   ```
   Writing token to Arduino...
   Token saved to slot #1
   ```

4. **Token Persists**
   - Token remains after power cycle

---

## Tips and Best Practices

### Power Management

- **USB Power:** Arduino can be powered via USB
- **Battery:** Flipper Zero battery depletes during use
- **Recommendation:** Keep both devices charged

### Connection Tips

- **Use quality USB cables:** Some cables are charge-only
- **Direct connection:** Avoid USB hubs when possible
- **Check connections:** Wiggle connectors if unstable

### Emulation Range

- **Optimal distance:** 0-5 cm from reader
- **Reader sensitivity:** Varies by model
- **Antenna orientation:** Rotate for best signal

### Token Security

- **Default keys:** Change from defaults for security
- **Document tokens:** Keep record of UIDs and keys
- **Test before deployment:** Verify on test reader first

### Troubleshooting Quick Reference

| Issue | Quick Fix |
|-------|-----------|
| Not connected | Check USB cable, press RIGHT to reconnect |
| Reader not responding | Move closer, rotate antenna |
| Token not saving | Check token slot not full |
| App crashes | Restart app, reboot Flipper |
| Wrong UID displayed | Select correct token from list |

### Keyboard Shortcuts

| Key | Function |
|-----|----------|
| OK | Open token list |
| UP | Toggle emulation |
| DOWN | Open debug view |
| LEFT | WriteFile token to Arduino |
| RIGHT | ReadFile token from Arduino |
| BACK | Return / Exit |

### FAQ

**Q: Can I store more than 10 tokens?**
A: No, the Arduino bridge has limited RAM. Use external storage for more tokens.

**Q: Why does the status show "Disconnected"?**
A: Check USB cable, Arduino power, and retry.

**Q: How do I change the authentication key?**
A: Use the token edit view or API commands.

**Q: Can I damage the hardware?**
A: The PIC32 has over-voltage protection. Use 3.3V logic only.

**Q: What's the maximum read range?**
A: Depends on reader power. Typically 0-10 cm.

---

## Technical Reference

### UART Protocol

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

### SPI Protocol (Arduino → PIC32)

| Parameter | Value |
|-----------|-------|
| Mode | SPI_MODE_0 |
| Clock | 4 MHz |
| Data Order | MSB first |
| Chip Select | Active low |

### Command Format

```
┌────┬────┬────┬────────┬────────┐
│ SOF│ CMD│LEN │ DATA   │ CHECK  │
│ 0x02│    │    │ LEN B  │ SUM    │
└────┴────┴────┴────────┴────────┘

Checksum = SOF ^ CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[n]
```

### Complete Command Set

| CMD | Name | Direction | Description |
|-----|------|-----------|-------------|
| 0x01 | PING | TX→RX | Test connection |
| 0x02 | RESET | TX→RX | Reset PIC32 state |
| 0x10 | LOAD_TOKEN | TX→RX | Load 32-byte token |
| 0x11 | SAVE_TOKEN | TX→RX | Get current token |
| 0x12 | LIST_TOKENS | TX→RX | List stored tokens |
| 0x13 | SELECT_TOKEN | TX→RX | Select active token |
| 0x20 | SET_UID | TX→RX | Set 32-bit UID |
| 0x21 | SET_KEY | TX→RX | Set 48-bit key |
| 0x22 | SET_CONFIG | TX→RX | Set configuration |
| 0x30 | GET_STATUS | TX→RX | Get system status |
| 0x40 | START_EMULATE | TX→RX | Start emulation |
| 0x41 | STOP_EMULATE | TX→RX | Stop emulation |

---

## Disclaimer

This user guide is for **educational and security research purposes only**. Always ensure proper authorization before testing on access control systems. Unauthorized access is illegal.

---

## See Also

- [Installation Guide](Installation_Guide.md) - Complete setup instructions
- [Troubleshooting Guide](Troubleshooting.md) - Common issues and solutions
- [HiTag2 Protocol](HiTag2_Protocol.md) - Protocol specification
- [Building Guide](Building.md) - Firmware build instructions
