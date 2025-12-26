# Hi-Tag 2 Protocol Reference

## Overview

Hi-Tag 2 is a 125 kHz RFID transponder protocol developed by NXP (formerly Philips). It features a 48-bit stream cipher for authentication and is commonly used in automotive immobilizers and access control systems like Paxton NET2.

## Physical Layer

### RF Characteristics

| Parameter | Value |
|-----------|-------|
| Carrier Frequency | 125 kHz |
| Modulation (Reader → Tag) | 100% ASK |
| Modulation (Tag → Reader) | BPSK |
| Bit Rate | 4 kbps (250 µs/bit) |
| Bit Period | 8 µs (Manchester half-bit) |

### Timing Requirements

| Parameter | Value |
|-----------|-------|
| Start Gap | 256 µs |
| Bit Period | 250 µs |
| Half-Bit Period | 8 µs |
| Response Delay | 256 µs |

## Memory Organization

Hi-Tag 2 has 8 pages of 32 bits each (256 bits total):

| Page | Address | Content |
|------|---------|---------|
| 0 | 0x00 | Serial Number (UID) - Read Only |
| 1 | 0x01 | Configuration |
| 2 | 0x02 | Secret Key (bits 0-31) |
| 3 | 0x03 | Secret Key (bits 32-47) + Password |
| 4-7 | 0x04-0x07 | User Data |

### Page 0 - Serial Number (32 bits)
```
[31:0] = Unique Identifier (UID)
```

### Page 1 - Configuration (32 bits)
```
[31:24] = Reserved
[23:16] = Configuration Byte
[15:0]  = Reserved
```

### Pages 2-3 - Secret Key (48 bits)
```
Page 2: [31:0]  = Key bits 0-31
Page 3: [15:0]  = Key bits 32-47
Page 3: [31:16] = Password (16 bits)
```

## Commands

### Standard Mode Commands

| Command | Code | Description |
|---------|------|-------------|
| START_AUTH | 11xxx | Start authentication |
| READ_PAGE | 11xxx | Read a memory page |
| WRITE_PAGE | 10xxx | Write a memory page |
| HALT | 00xxx | Halt transponder |

### Authentication Sequence

1. **Reader sends START_AUTH command**
   - 5-bit command + 32-bit challenge

2. **Tag responds with**
   - 32-bit UID
   - 32-bit authentication response

3. **Reader verifies response**
   - Uses shared 48-bit key
   - Computes expected response
   - Compares with received response

## Stream Cipher

Hi-Tag 2 uses a 48-bit Linear Feedback Shift Register (LFSR) based stream cipher.

### Cipher Initialization

```
State[47:0] = Key[47:0] XOR (UID[31:0] || Challenge[31:0])
```

### LFSR Feedback Polynomial

```
x^48 + x^43 + x^38 + x^18 + x^17 + x^14 + x^12 + x^7 + x^5 + 1
```

### Output Function

The cipher output is a non-linear function of specific LFSR bits:
```
Output = f(State[0], State[2], State[3], State[6], State[7], State[8], ...)
```

## Paxton NET2 Specifics

Paxton NET2 systems use Hi-Tag 2 transponders with specific configurations:

### Token Format
- Standard Paxton tokens use 32-bit site codes
- User ID encoded in user data pages
- Site-specific encryption keys

### Communication Flow

```
1. Reader powers field (125 kHz)
2. Token enters field, draws power
3. Reader sends authentication challenge
4. Token responds with UID + encrypted response
5. Reader verifies authentication
6. If valid, reader reads user data
7. Access decision made based on user ID
```

## Implementation Notes

### Timing Critical Operations

For successful Hi-Tag 2 emulation, the following timing requirements must be met:

| Operation | Timing | Tolerance |
|-----------|--------|-----------|
| Bit period | 8 µs | ±0.5 µs |
| Response delay | 256 µs | ±10 µs |
| Modulation depth | 100% ASK | N/A |

### PIC32 Implementation

At 80 MHz, the PIC32 provides:
- 12.5 ns instruction cycle
- 640 cycles per 8 µs bit period
- Sufficient resolution for precise timing

### Crypto Implementation

The 48-bit stream cipher can be implemented efficiently:
- LFSR state fits in 64-bit variable
- Feedback computation: ~10 cycles
- Output function: ~20 cycles
- Total per bit: ~50 cycles (well within budget)

## Security Considerations

⚠️ **Warning**: Hi-Tag 2's security has been compromised.

Known vulnerabilities:
1. **Weak cipher** - 48-bit key space is brute-forceable
2. **Crypto attacks** - Algebraic attacks can recover keys
3. **Replay attacks** - Possible in some configurations

This emulator is intended for:
- Security research
- Penetration testing (with authorization)
- Educational purposes

**Do not use for unauthorized access.**

## References

1. Garcia, F.D., et al. "Dismantling HITAG2" (2012)
2. NXP HITAG2 Datasheet
3. RFIDler Source Code - hitag2.c
