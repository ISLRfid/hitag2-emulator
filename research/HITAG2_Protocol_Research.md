# Hi-Tag 2 Protocol - Comprehensive Research Document

## Table of Contents

1. [Introduction](#introduction)
2. [Physical Layer Specifications](#physical-layer-specifications)
3. [Data Encoding and Modulation](#data-encoding-and-modulation)
4. [Memory Architecture](#memory-architecture)
5. [Command Set](#command-set)
6. [Authentication Protocol](#authentication-protocol)
7. [Stream Cipher Implementation](#stream-cipher-implementation)
8. [Timing Specifications](#timing-specifications)
9. [Paxton NET2 Integration](#paxton-net2-integration)
10. [Security Analysis](#security-analysis)
11. [Implementation Considerations](#implementation-considerations)
12. [References](#references)

---

## Introduction

### Overview

Hi-Tag 2 is a proprietary 125 kHz RFID transponder protocol originally developed by Philips Semiconductors (now NXP Semiconductors). It has been widely deployed in automotive immobilizer systems, access control systems, and animal identification applications since the late 1990s.

The protocol features a custom 48-bit stream cipher for authentication, which was considered secure at the time of its design but has since been found to have significant vulnerabilities. This document provides comprehensive technical details for security researchers and developers working with the Hi-Tag 2 protocol.

### Key Characteristics

| Characteristic | Description |
|----------------|-------------|
| Frequency | 125 kHz (LF band) |
| Data Rate | ~4 kbps |
| Memory | 256 bits (8 pages × 32 bits) |
| Cipher | 48-bit LFSR stream cipher |
| Authentication | Challenge-response with 48-bit key |
| Read Range | Up to 10 cm (passive tag) |

### Historical Context

Hi-Tag 2 was designed as an improvement over the original Hi-Tag protocol, adding cryptographic authentication to prevent cloning. It became particularly popular in:

- **Automotive**: Used in many European car immobilizer systems (Peugeot, Citroën, Renault, Fiat)
- **Access Control**: Deployed in commercial and residential systems
- **Animal Identification**: ISO 11785 compliant variants exist
- **Paxton NET2**: Specific configuration for access control systems

---

## Physical Layer Specifications

### Carrier Signal

The Hi-Tag 2 system operates in the Low Frequency (LF) band at 125 kHz. This frequency was chosen for:

- Good penetration through materials (plastic, fabric, human tissue)
- Relatively long range compared to HF systems
- Lower power consumption for passive tags
- Well-established antenna designs

#### RF Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Center Frequency | 125 kHz | ±5 kHz tolerance |
| Frequency Deviation | N/A | CW carrier, no FSK |
| Field Strength | 100-500 mV/m | Reader TX strength |
| Modulation Depth | 100% | Reader to tag (ASK) |
| Bit Rate | 4 kbps | 250 µs per bit |

### Antenna Design

The 125 kHz antenna coil typically has the following characteristics:

```
L = 150 µH (typical)
Q = 50-100 (quality factor)
Diameter = 30-50 mm (reader)
Diameter = 5-10 mm (tag)
```

**Resonant Circuit Calculation:**

For a parallel resonant circuit at 125 kHz:

```
f = 1 / (2π × √(L × C))

Given L = 150 µH:
C = 1 / (4π² × f² × L)
C = 1 / (4π² × (125,000)² × 150e-6)
C ≈ 10.8 nF (typical value)
```

### Power Harvesting

Passive Hi-Tag 2 tags derive their operating power from the reader's magnetic field:

```
P_available = (V_field² × Q) / (ω × L)

Where:
- V_field = induced voltage
- Q = antenna quality factor
- ω = angular frequency (2πf)
- L = antenna inductance
```

Typical tag operating voltage: 2.0-5.0V (regulated to 3.3V or 5V internally)

---

## Data Encoding and Modulation

### Reader to Tag (Downlink)

The reader communicates to the tag using **100% Amplitude Shift Keying (ASK)** with **Manchester encoding**.

#### 100% ASK Modulation

```
Logic 0: Full carrier ON
Logic 1: Carrier OFF (no transmission)

This means:
- Data '0' = 100% modulation depth
- Data '1' = 0% modulation depth (silence)
```

This encoding scheme is also known as "OOK" (On-Off Keying) with Manchester subcarrier.

#### Manchester Encoding

Manchester encoding combines clock and data by encoding each bit as a transition:

```
Logic 0: High-to-Low transition in middle of bit period
Logic 1: Low-to-High transition in middle of bit period

Bit Period = 250 µs
Half-Bit Period = 125 µs
```

**Manchester Encoding Example (Data: 0101):**

```
Data:     0     1     0     1
          │     │     │     │
          ▼     │     ▼     │
Carrier: ▓▓▓▓▓  ────  ▓▓▓▓▓  ────
         │←125µs→│←125µs→│

Pattern: ▓▓▓▓▓───────▓▓▓▓▓───────
```

### Tag to Reader (Uplink)

The tag responds to the reader using **BPSK (Binary Phase Shift Keying)** on a subcarrier:

#### BPSK Modulation

```
Logic 0: 180° phase shift
Logic 1: 0° phase shift (no phase shift)

Subcarrier Frequency: 4 kHz (same as bit rate)
```

#### Subcarrier Generation

The tag generates a 4 kHz subcarrier from the 125 kHz carrier:

```
f_subcarrier = f_carrier / 32
f_subcarrier = 125,000 / 32 ≈ 3,906 Hz ≈ 4 kHz
```

### Complete Signal Flow

```
Reader TX (125 kHz ASK):
├─ Start Gap: 256 µs (no carrier)
├─ Command + Data: Manchester encoded
└─ Each bit: 250 µs duration

Tag RX:
├─ Demodulates 100% ASK
├─ Extracts clock from Manchester
└─ Decodes data bits

Tag TX (125 kHz with BPSK):
├─ Generate 4 kHz subcarrier
├─ BPSK modulate subcarrier on carrier
└─ Send response after 256 µs delay
```

---

## Memory Architecture

### Memory Organization

Hi-Tag 2 provides 256 bits of non-volatile memory organized as 8 pages of 32 bits each:

```
┌────────────────────────────────────────┐
│ Page 0  │ Serial Number (UID)     [R]  │  0x00
├────────────────────────────────────────┤
│ Page 1  │ Configuration           [R/W]│  0x01
├────────────────────────────────────────┤
│ Page 2  │ Secret Key (bits 0-31)  [R]  │  0x02
├────────────────────────────────────────┤
│ Page 3  │ Secret Key (bits 32-47) [R]  │  0x03
│         │ Password (bits 48-63)   [R/W]│
├────────────────────────────────────────┤
│ Page 4  │ User Data               [R/W]│  0x04
├────────────────────────────────────────┤
│ Page 5  │ User Data               [R/W]│  0x05
├────────────────────────────────────────┤
│ Page 6  │ User Data               [R/W]│  0x06
├────────────────────────────────────────┤
│ Page 7  │ User Data               [R/W]│  0x07
└────────────────────────────────────────┘
[R] = Read-only
[R/W] = Read-write
```

### Page 0: Serial Number (UID)

The unique identifier is factory-programmed and read-only:

```
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
└───────────────────────────────────────────────────────────────────────────────────────────────┘
                                    Unique Identifier (32 bits)
```

**Format:**
- Manufacturer code (first 8 bits)
- Unique serial number (remaining 24 bits)

### Page 1: Configuration

Configuration byte settings:

```
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
     Reserved (24 bits)     │  Config   │              Reserved (16 bits)
                            └───────────┘
```

**Configuration Byte (bits 23-16):**

| Bit | Function | Description |
|-----|----------|-------------|
| 7 | RFU | Reserved for future use |
| 6 | RFU | Reserved for future use |
| 5 | RFU | Reserved for future use |
| 4 | RFU | Reserved for future use |
| 3 | Antenna Tuning | 0 = Standard, 1 = Extended range |
| 2 | Read/Write Lock | 0 = Unlocked, 1 = Page 4-7 locked |
| 1 | Authentication Required | 0 = No, 1 = Yes for read access |
| 0 | Password Required | 0 = No, 1 = Password needed for write |

### Pages 2-3: Secret Key

The 48-bit authentication key is stored in pages 2 and 3:

```
Page 2 (Address 0x02):
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
                                    Key[0:31] (LSB = Key bit 0)
```

```
Page 3 (Address 0x03):
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
    Password[15:0] (16 bits)      │         Key[32:47] (16 bits)          │
                                  └───────────────────────────────────────┘
                                  Key[47:32] (MSB = Key bit 47)
```

### Pages 4-7: User Data

These 4 pages (128 bits) are available for user application data:

| Page | Size | Typical Use |
|------|------|-------------|
| 4 | 32 bits | Application specific |
| 5 | 32 bits | Application specific |
| 6 | 32 bits | Application specific |
| 7 | 32 bits | Application specific |

**Paxton NET2 Usage:**

```
Page 4: Site Code (32-bit)
Page 5: User ID (32-bit)
Page 6: Additional data (32-bit)
Page 7: Configuration/Signature (32-bit)
```

---

## Command Set

### Command Encoding

All commands are 5 bits long, followed by optional address or data:

```
┌─────────────────────────────────────────────────────────────────────────┐
│  4   │  3   │  2   │  1   │  0   │  A4  │ A3  │ A2  │ A1  │ A0  │ D7..D0 │
│ MSB  │      │      │      │  LSB │  Address (5 bits)  │  Data (8 bits) │
└─────────────────────────────────────────────────────────────────────────┘
   5-bit Command Code              5-bit Address    8-bit Data
```

### Command Summary

| Command | Code | Format | Description |
|---------|------|--------|-------------|
| TEST_RST | 00000 | - | Test/Reset |
| READ_PAGE | 110AA | 32 bits | Read page at address AA |
| WRITE_PAGE | 100AA | 32 bits | Write 32 bits to page |
| START_AUTH | 111AA | 32 bits | Start authentication |
| HALT | 00000 | - | Enter halt mode |

### Detailed Command Descriptions

#### TEST_RST (0x00)

The TEST_RST command resets the tag to its initial state:

```
Sequence:
1. Reader sends 5-bit command (00000)
2. Tag resets internal state
3. Tag waits for next command
```

#### READ_PAGE (0x60-0x7F)

Read a 32-bit page from memory:

```
Command Format:
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│  1  │  1  │  0  │ A4  │ A3  │ A2  │ A1  │ A0  │     │     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
   5-bit command              5-bit page address

Response Format:
┌─────────────────────────────────────────────────────────────┐
│                    32-bit Page Data                         │
└─────────────────────────────────────────────────────────────┘
```

**Page Address Encoding:**

| Page | A4 | A3 | A2 | A1 | A0 | Command |
|------|----|----|----|----|----|---------|
| 0 | 0 | 0 | 0 | 0 | 0 | 0x60 |
| 1 | 0 | 0 | 0 | 0 | 1 | 0x61 |
| 2 | 0 | 0 | 0 | 1 | 0 | 0x62 |
| 3 | 0 | 0 | 0 | 1 | 1 | 0x63 |
| 4 | 0 | 0 | 1 | 0 | 0 | 0x64 |
| 5 | 0 | 0 | 1 | 0 | 1 | 0x65 |
| 6 | 0 | 0 | 1 | 1 | 0 | 0x66 |
| 7 | 0 | 0 | 1 | 1 | 1 | 0x67 |

#### WRITE_PAGE (0x40-0x5F)

Write 32 bits to a page:

```
Command Format:
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│  1  │  0  │  0  │ A4  │ A3  │ A2  │ A1  │ A0  │ D7..D0 (32 bits)     │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────────────────────┘
   5-bit command              5-bit page address   32-bit data

Write Sequence:
1. Reader sends command + address
2. Reader sends 32 bits of data (LSB first)
3. Tag verifies data
4. Tag acknowledges write
```

**Write Address Encoding:**

| Page | A4 | A3 | A2 | A1 | A0 | Command |
|------|----|----|----|----|----|---------|
| 0 | 0 | 0 | 0 | 0 | 0 | 0x40 (RO) |
| 1 | 0 | 0 | 0 | 0 | 1 | 0x41 |
| 2 | 0 | 0 | 0 | 1 | 0 | 0x42 (RO) |
| 3 | 0 | 0 | 0 | 1 | 1 | 0x43 (RO) |
| 4 | 0 | 0 | 1 | 0 | 0 | 0x44 |
| 5 | 0 | 0 | 1 | 0 | 1 | 0x45 |
| 6 | 0 | 0 | 1 | 1 | 0 | 0x46 |
| 7 | 0 | 0 | 1 | 1 | 1 | 0x47 |

#### START_AUTH (0x70-0x7F)

Initiate authentication sequence:

```
Command Format:
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────────────────────┐
│  1  │  1  │  1  │ A4  │ A3  │ A2  │ A1  │ A0  │ 32-bit Challenge    │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────────────────────┘
   5-bit command              5-bit page address   Reader challenge

Response Format:
┌───────────────────────────────────────────────────────────────────────┐
│                    32-bit UID (Page 0)                                │
├───────────────────────────────────────────────────────────────────────┤
│                    32-bit Authentication Response                      │
└───────────────────────────────────────────────────────────────────────┘
```

**Auth Address:**

| Address | Meaning |
|---------|---------|
| 0x00-0x07 | Challenge page selection (typically 0x00) |

#### HALT (0x00)

Enter low-power halt mode:

```
Command Format:
┌─────┬─────┬─────┬─────┬─────┐
│  0  │  0  │  0  │  0  │  0  │
└─────┴─────┴─────┴─────┴─────┘
   5-bit command

Effect:
- Tag stops responding to commands
- Requires power cycle or specific wake sequence to exit
```

---

## Authentication Protocol

### Challenge-Response Mechanism

Hi-Tag 2 uses a challenge-response authentication protocol to verify tag authenticity:

```
┌──────────────┐                                                   ┌──────────────┐
│    Reader    │                                                   │     Tag      │
└──────┬───────┘                                                   └──────┬───────┘
       │                                                                │
       │  1. START_AUTH (5 bits) + Challenge[31:0]                     │
       │───────────────────────────────────────────────────────────────>
       │                                                                │
       │                                                      2. Compute response
       │                                                      using 48-bit key
       │                                                                │
       │  3. UID[31:0] + Response[31:0]                                │
       │<───────────────────────────────────────────────────────────────
       │                                                                │
       │  4. Verify response using shared secret key                   │
       │      (Reader has copy of tag's key)                           │
       │                                                                │
```

### Authentication Steps

#### Step 1: Reader Sends Challenge

The reader generates a random 32-bit challenge and sends it to the tag:

```
START_AUTH command: 111AA (where AA = address)
32-bit challenge:   R[31:0] (random from reader)
```

#### Step 2: Tag Computes Response

The tag computes the response using the stream cipher:

```
LFSR State Initialization:
State[47:0] = Key[47:0] XOR (UID[31:0] || Challenge[31:0])

Stream Generation:
For i = 0 to 31:
    Stream[i] = LFSR_output(State)
    State = LFSR_advance(State)

Response[31:0] = Stream[31:0]
```

#### Step 3: Tag Sends Response

The tag responds with:

```
UID[31:0]      = Page 0 content
Response[31:0] = 32-bit encrypted response
```

#### Step 4: Reader Verifies

The reader verifies the response:

```
1. Read UID from database
2. Look up secret key for this UID
3. Compute expected response
4. Compare with received response
5. Accept or reject authentication
```

### Key Diversification

Many systems use key diversification to avoid storing the same key on all tags:

```
Diversified Key = f(UID, Master Key)

Common diversification methods:
1. Simple XOR: Key = Master_Key XOR UID
2. AES-based: Key = AES_Encrypt(Master_Key, UID)
3. Custom: Proprietary function
```

---

## Stream Cipher Implementation

### LFSR Architecture

The Hi-Tag 2 cipher uses a 48-bit Linear Feedback Shift Register (LFSR) with a custom feedback polynomial.

#### LFSR Structure

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              48-bit LFSR                                      │
├────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┤
│ S47│ S46│ S45│ S44│ S43│ S42│ S41│ S40│ S39│ S38│ S37│ S36│ S35│ S34│ S33│ S32│
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ S31│ S30│ S29│ S28│ S27│ S26│ S25│ S24│ S23│ S22│ S21│ S20│ S19│ S18│ S17│ S16│
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ S15│ S14│ S13│ S12│ S11│ S10│ S9 │ S8 │ S7 │ S6 │ S5 │ S4 │ S3 │ S2 │ S1 │ S0 │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
      │            │                      │                    │
      └────────────┴──────────────────────┴────────────────────┘
                   Tap Points
```

#### Feedback Polynomial

The LFSR uses the following feedback polynomial (represented in hexadecimal):

```
P(x) = x^48 + x^43 + x^38 + x^18 + x^17 + x^14 + x^12 + x^7 + x^5 + 1

Tap positions (0-indexed, LSB to MSB):
- Bit 0 (constant)
- Bit 5
- Bit 7
- Bit 12
- Bit 14
- Bit 17
- Bit 18
- Bit 38
- Bit 43
```

**Tap Bitmask (hex):**

```
TAPS = 0x0000_0000_0800_0429  (bits: 0, 5, 7, 12, 14, 17, 18, 38, 43)
```

#### State Update Function

Each clock cycle:

```
Output_bit = S0  (LSB is output)

New_S47 = S0 XOR S5 XOR S7 XOR S12 XOR S14 XOR S17 XOR S18 XOR S38 XOR S43
Shift right by 1 (toward LSB)
```

### Non-Linear Output Function

The cipher output is not directly from the LFSR but uses a non-linear function:

```
Output = f(S[0], S[2], S[3], S[6], S[7], S[8], S[10], S[15], ...)
```

This adds confusion to the cipher output, making algebraic attacks more difficult.

### Complete Encryption Process

```python
def hitag2_crypt(key, uid, challenge, num_bits=32):
    """
    Hi-Tag 2 stream cipher encryption.
    
    Args:
        key: 48-bit authentication key (integer)
        uid: 32-bit tag identifier (integer)
        challenge: 32-bit reader challenge (integer)
        num_bits: Number of bits to generate
    
    Returns:
        32-bit authentication response (integer)
    """
    # Initialize LFSR state
    state = key ^ ((uid << 32) | challenge)
    
    # Generate keystream
    response = 0
    for i in range(num_bits):
        # Compute output bit using non-linear function
        out_bit = ((state >> 0) & 1) ^ ((state >> 2) & 1)
        out_bit ^= ((state >> 3) & 1) ^ ((state >> 6) & 1)
        out_bit ^= ((state >> 7) & 1) ^ ((state >> 8) & 1)
        
        # Shift state
        feedback = ((state >> 0) & 1) ^ ((state >> 5) & 1)
        feedback ^= ((state >> 7) & 1) ^ ((state >> 12) & 1)
        feedback ^= ((state >> 14) & 1) ^ ((state >> 17) & 1)
        feedback ^= ((state >> 18) & 1) ^ ((state >> 38) & 1)
        feedback ^= ((state >> 43) & 1)
        
        state = (feedback << 47) | (state >> 1)
        
        # Build response
        response = (response << 1) | out_bit
    
    return response
```

---

## Timing Specifications

### Critical Timing Parameters

All timing must be met precisely for reliable communication:

| Parameter | Symbol | Nominal Value | Tolerance | Units |
|-----------|--------|---------------|-----------|-------|
| Start Gap | T_gap | 256 | ±16 | µs |
| Bit Period | T_bit | 250 | ±10 | µs |
| Half-Bit Period | T_half | 125 | ±5 | µs |
| Response Delay | T_delay | 256 | ±16 | µs |
| Modulation Rise Time | T_rise | <1 | - | µs |
| Modulation Fall Time | T_fall | <1 | - | µs |

### Timing Diagram

```
Reader TX (ASK):
         ┌─────────────────────┐
Carrier: │                     │
         └─────────────────────┘
              ↑
              │ T_gap = 256 µs
              │
         ┌────┴────┐
         │ Command │
         │ 5 bits  │
         └─────────┘
              │
              │ T_bit = 250 µs per bit
              │
         ┌────┴────┐
         │  Data   │
         │ N bits  │
         └─────────┘

Tag RX Window:
              │← 256 µs window →│
              │                 │
         <───▼─────────────────▼──────>
         Gap ends            First bit
                            edge expected

Tag TX (BPSK):
              ← T_delay = 256 µs →
              │              ┌─────┐
Response:     │              │     │
              │              └─────┘
              │                 ↑
              │                 │ T_bit = 250 µs
         Gap ends            Response starts
```

### Timing Budget Analysis

For the PIC32MX795F512L running at 80 MHz:

```
Clock period = 1 / 80 MHz = 12.5 ns

For T_bit = 250 µs:
- Cycles available = 250 µs / 12.5 ns = 20,000 cycles
- Margin for jitter: ±10 µs = 800 cycles (±4%)

For T_half = 125 µs:
- Cycles available = 125 µs / 12.5 ns = 10,000 cycles

For T_delay = 256 µs:
- Cycles available = 256 µs / 12.5 ns = 20,480 cycles
```

### Inter-Symbol Timing

```
Gap Timing:
├─ Minimum gap: 240 µs
├─ Nominal gap: 256 µs
└─ Maximum gap: 272 µs

If gap outside range:
- Tag may not detect command start
- Tag may synchronize incorrectly
- Communication failure results
```

---

## Paxton NET2 Integration

### Overview

Paxton NET2 is a commercial access control system that uses Hi-Tag 2 transponders. Understanding the specific implementation is crucial for emulation.

### Token Structure

NET2 tokens use the Hi-Tag 2 memory as follows:

```
┌─────────────────────────────────────────────────────────────────────────┐
│ Page │ Content                              │ Access Level              │
├─────────────────────────────────────────────────────────────────────────┤
│  0   │ UID (32-bit)                         │ Read-only, factory prog.  │
│  1   │ Configuration                        │ Read-only, factory prog.  │
│  2   │ Key bits 0-31                        │ Read-only, factory prog.  │
│  3   │ Key bits 32-47 + Password            │ Read-only, factory prog.  │
│  4   │ Site Code (32-bit)                   │ Read/write                │
│  5   │ User ID (32-bit)                     │ Read/write                │
│  6   │ Access Flags (32-bit)                │ Read/write                │
│  7   │ Reserved/Extended                    │ Reserved                  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Authentication Flow

```
1. Reader powers up
2. Reader sends TEST_RST to all tags in field
3. Reader sends START_AUTH + Challenge (random 32-bit)
4. Tag responds with UID + Response
5. Reader looks up UID in database
6. Reader retrieves site code and key for UID
7. Reader computes expected response
8. Reader compares with received response
9. If valid, reader reads Pages 4-6 (Site Code, User ID, Access Flags)
10. Reader makes access decision based on User ID and Access Flags
```

### Site Code Format

The 32-bit site code identifies the installation:

```
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
└─────────────────────┬─────────────────────┘
      Site ID (high)          Site ID (low)
```

### User ID Format

```
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
└───────────────────────────────┬───────────────────────────────┘
         User Number (high)                User Number (low)
```

### Access Flags

```
┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│31│30│29│28│27│26│25│24│23│22│21│20│19│18│17│16│15│14│13│12│11│10│ 9│ 8│ 7│ 6│ 5│ 4│ 3│ 2│ 1│ 0│
└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
   RFU   │     Door Group     │ RFU │     TZ1-TZ4 Time Zones      │
         (4 bits)                  (4 bits × 4 zones)
```

### Key Configuration

NET2 systems typically use:

- **Site-specific keys**: Each installation has a unique key
- **Key diversification**: Key derived from site code + master key
- **Read-protected pages**: Pages 2-3 (key storage) are read-only

---

## Security Analysis

### Known Vulnerabilities

#### 1. Weak Key Space

The 48-bit key space is too small for modern security:

```
Key space: 2^48 = 281,474,976,710,656 possible keys
Brute-force time (at 10,000 keys/sec): ~8,913 years
Brute-force time (at 1M keys/sec): ~89 years
```

With distributed computing and optimized implementations, practical attacks are possible.

#### 2. Algebraic Attacks

The LFSR structure allows algebraic attacks:

```
Complexity: O(2^24) operations for full key recovery
Reference: "Dismantling HITAG2" - Garcia et al., 2012
```

#### 3. Ciphertext-Only Attack

With enough authentication exchanges:

```
1. Capture many (challenge, response) pairs
2. Set up system of equations
3. Solve for key bits
4. Recover full 48-bit key
```

#### 4. Side-Channel Attacks

Physical implementations may leak information through:

- Power consumption variations
- Electromagnetic emissions
- Timing differences

### Attack Complexity Summary

| Attack Type | Requirements | Complexity | Practicality |
|-------------|--------------|------------|--------------|
| Brute force | None | 2^48 | Not practical |
| Algebraic | 32+ challenges | ~2^24 | Feasible |
| Ciphertext-only | Many traces | ~2^24 | Feasible |
| Side-channel | Physical access | Varies | Highly practical |
| Cloning | Key recovery | - | With key, trivial |

### Defensive Measures

While this emulator is for research, legitimate uses should:

1. **Use strong keys**: Avoid default or sequential keys
2. **Implement rate limiting**: Limit authentication attempts
3. **Use diversified keys**: Derive keys from UID
4. **Regular key rotation**: Change keys periodically
5. **Physical security**: Protect readers from tampering

---

## Implementation Considerations

### Hardware Requirements

For Hi-Tag 2 emulation, the following hardware capabilities are needed:

#### Timing Precision

```
Required resolution: <1 µs
Desired resolution: <100 ns

At 80 MHz PIC32:
- Clock period: 12.5 ns
- 8 µs bit = 640 clock cycles
- Substantial margin for code execution
```

#### RF Generation

```
Carrier: 125 kHz square wave or sine
Precision: ±500 Hz (±0.4%)
Modulation: 100% ASK (ON/OFF)
Subcarrier: 4 kHz for tag response
```

### Firmware Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Firmware Architecture                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────────────────┐   │
│  │  UART RX    │────►│ Command     │────►│ RF Transmitter (125kHz) │   │
│  │  (from PC)  │     │ Parser      │     │ - Modulation            │   │
│  └─────────────┘     └──────┬──────┘     │ - Manchester encoding   │   │
│                            │             └─────────────────────────┘   │
│                            │                                              │
│                            ▼                                              │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────────────────┐   │
│  │  UART TX    │◄────│ Response    │◄────│ RF Receiver (125kHz)    │   │
│  │  (to PC)    │     │ Generator   │     │ - BPSK demodulation     │   │
│  └─────────────┘     └──────┬──────┘     │ - Clock recovery        │   │
│                            │             └─────────────────────────┘   │
│                            │                                              │
│                            ▼                                              │
│                     ┌─────────────┐                                       │
│                     │ Crypto      │                                       │
│                     │ Engine      │                                       │
│                     │ - LFSR      │                                       │
│                     │ - Key stream│                                       │
│                     └─────────────┘                                       │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Code Optimization

For real-time crypto operations:

```c
// Fast LFSR advance (one cycle)
uint64_t hitag2_lfsr_advance(uint64_t state) {
    uint64_t feedback = (state ^ (state >> 5) ^ (state >> 7) ^ 
                        (state >> 12) ^ (state >> 14) ^ (state >> 17) ^
                        (state >> 18) ^ (state >> 38) ^ (state >> 43)) & 1;
    return (feedback << 47) | (state >> 1);
}

// Generate 32-bit response
uint32_t hitag2_response(uint48_t key, uint32_t uid, uint32_t challenge) {
    uint64_t state = key ^ ((uint64_t)uid << 32) | challenge;
    uint32_t response = 0;
    
    for (int i = 0; i < 32; i++) {
        // Non-linear output function
        uint64_t out = (state ^ (state >> 2) ^ (state >> 3) ^ 
                       (state >> 6) ^ (state >> 7) ^ (state >> 8));
        response = (response << 1) | (out & 1);
        state = hitag2_lfsr_advance(state);
    }
    
    return response;
}
```

### Memory Requirements

```
Flash:  32-64 KB (firmware + lookup tables)
RAM:    8-16 KB (state buffers, stack)
EEPROM: N/A (emulated in flash)
```

---

## References

### Academic Papers

1. **"Dismantling HITAG2"** - Garcia, F.D., et al.
   - Conference: 27th Chaos Communication Congress (27C3), 2012
   - URL: https://github.com/FDG/ECCEH12
   - Summary: Complete cryptanalysis of Hi-Tag 2, key recovery attacks

2. **"A Practical Attack on HITAG2"** - Courtois, N.T.
   - Conference: 4th Workshop on Cryptography for Embedded Systems, 2014
   - Summary: Algebraic attacks using SAT solvers

### Technical Documentation

3. **NXP HITAG2 Datasheet**
   - Original protocol specification
   - Now available through reverse engineering

4. **RFIDler Project** - Adam Laurie
   - URL: https://github.com/AdamLaurie/RFIDler
   - Reference implementation of Hi-Tag 2 emulation

### Tools and Resources

5. **Proxmark3** - RFID Security Research Tool
   - URL: https://github.com/Proxmark/proxmark3
   - Supports Hi-Tag 2 capture and replay

6. **Flipper Zero** - Multi-tool for RFID
   - URL: https://flipperzero.one/
   - Community support for Hi-Tag 2

7. **Mifare Classic Tool** - Android Application
   - Includes Hi-Tag 2 support for reading/copying

### Related Standards

8. **ISO 11785** - Radio frequency identification of animals
   - Technical concept for LF RFID
   - Similar modulation schemes

9. **ISO 14443** - Proximity cards
   - Different protocol family (HF, 13.56 MHz)
   - Comparison reference for RFID protocols

---

## Appendix A: Command Reference

### Quick Command Reference

| Command | Binary | Hex | Parameters | Response |
|---------|--------|-----|------------|----------|
| TEST_RST | 00000 | 0x00 | None | None |
| READ_PAGE | 110aa | 0x60-0x67 | Page address (5 bits) | 32-bit data |
| WRITE_PAGE | 100aa | 0x40-0x47 | Page address (5 bits) | 32-bit data |
| START_AUTH | 111aa | 0x70-0x77 | Page address (5 bits) | UID + Response |
| HALT | 00000 | 0x00 | None | None |

### Response Codes

The tag does not provide explicit response codes. Instead:

- **Successful command**: Tag sends expected data or ACK pattern
- **Failed command**: Tag sends all-zeros or no response
- **Authentication failure**: Tag may still respond but with incorrect data

---

## Appendix B: Bit Ordering

### Least Significant Bit (LSB) First

All data is transmitted LSB first:

```
Example: Value 0x12345678 (binary: 0001 0010 0011 0100 0101 0110 0111 1000)

Transmit order:
Bit 0 (LSB): 0
Bit 1:       0
Bit 2:       0
...
Bit 31 (MSB): 0
```

### Page Data Example

```
Page 0 contains: 0xA5B6C7D8

Binary: 1010 0101 1011 0110 1100 0111 1101 1000
LSB first transmission: 0001 1011 1110 0110 1101 1010 0101 01
```

---

## Appendix C: Glossary

| Term | Definition |
|------|------------|
| ASK | Amplitude Shift Keying - modulation by varying amplitude |
| BPSK | Binary Phase Shift Keying - modulation by inverting phase |
| LFSR | Linear Feedback Shift Register - shift register with feedback taps |
| UID | Unique Identifier - factory-programmed tag serial number |
| Challenge | Random data sent by reader for authentication |
| Response | Encrypted response computed using secret key |
| Reader | The interrogator device that powers and communicates with tags |
| Tag | The transponder (passive or active) that stores data |
| Carrier | The continuous RF signal that carries data |
| Subcarrier | Lower frequency signal used for tag response |

---

## Document Information

| Property | Value |
|----------|-------|
| Version | 1.0 |
| Author | Hi-Tag 2 Emulator Project |
| Date | 2024 |
| License | MIT |
