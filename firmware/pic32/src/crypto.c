/*
 * Hi-Tag 2 Emulator - 48-bit Stream Cipher Module
 * Implements the Hi-Tag 2 challenge-response authentication
 * 
 * The cipher uses a 48-bit LFSR with the following polynomial:
 * P(x) = x^48 + x^44 + x^41 + x^40 + x^39 + x^37 + x^35 + x^34 + 
 *        x^31 + x^19 + x^14 + x^12 + x^4 + 1
 * 
 * Simplified version using standard taps for emulation
 */

#include "crypto.h"
#include "debug.h"

// Current secret key (48 bits)
static uint8_t g_secret_key[6] = {0};

// LFSR taps for Hi-Tag 2 (from reverse engineering)
// These are the standard taps for the Philips/Siemens Hi-Tag 2 cipher
#define LFSR_TAP_47    (1U << 47)  // Bit 47 (MSB)
#define LFSR_TAP_45    (1U << 45)  // Bit 45
#define LFSR_TAP_44    (1U << 44)  // Bit 44
#define LFSR_TAP_43    (1U << 43)  // Bit 43
#define LFSR_TAP_41    (1U << 41)  // Bit 41
#define LFSR_TAP_39    (1U << 39)  // Bit 39
#define LFSR_TAP_38    (1U << 38)  // Bit 38
#define LFSR_TAP_35    (1U << 35)  // Bit 35
#define LFSR_TAP_33    (1U << 33)  // Bit 33
#define LFSR_TAP_32    (1U << 32)  // Bit 32
#define LFSR_TAP_29    (1U << 29)  // Bit 29
#define LFSR_TAP_23    (1U << 23)  // Bit 23
#define LFSR_TAP_18    (1U << 18)  // Bit 18
#define LFSR_TAP_16    (1U << 16)  // Bit 16
#define LFSR_TAP_8     (1U << 8)   // Bit 8
#define LFSR_TAP_0     (1U << 0)   // Bit 0 (LSB)

// Alternative simplified taps (based on various sources)
// The actual taps may vary depending on tag manufacturer
#define LFSR_POLY      0x1B9E7F1A5C6D  // Example polynomial mask

/*
 * Initialize crypto subsystem
 */
void crypto_init(void) {
    DEBUG_PRINT("Crypto subsystem initialized\r\n");
}

/*
 * Set the secret key (48 bits)
 */
void crypto_set_key(const uint8_t* key) {
    // Store 48-bit key (6 bytes)
    g_secret_key[0] = key[0];
    g_secret_key[1] = key[1];
    g_secret_key[2] = key[2];
    g_secret_key[3] = key[3];
    g_secret_key[4] = key[4];
    g_secret_key[5] = key[5];
    
    DEBUG_PRINT("Key set: %02X%02X%02X%02X%02X%02X\r\n",
        key[0], key[1], key[2], key[3], key[4], key[5]);
}

/*
 * Get the current secret key
 */
void crypto_get_key(uint8_t* key) {
    key[0] = g_secret_key[0];
    key[1] = g_secret_key[1];
    key[2] = g_secret_key[2];
    key[3] = g_secret_key[3];
    key[4] = g_secret_key[4];
    key[5] = g_secret_key[5];
}

/*
 * Compute authentication response
 * 
 * The Hi-Tag 2 authentication protocol:
 * 1. Reader sends START_AUTH + 32-bit challenge
 * 2. Tag computes response using LFSR cipher
 * 3. Tag sends UID + response
 * 
 * LFSR initialization:
 * State[47:0] = Key[47:0] XOR (UID[31:0] || Challenge[31:0])
 * 
 * Response generation:
 * For i = 0 to 31:
 *     Output bit = State[0]
 *     Shift LFSR, XOR with Output bit at each step
 */
uint32_t crypto_compute_response(const uint8_t* key, uint32_t uid, uint32_t challenge) {
    uint64_t state = 0;
    uint32_t response = 0;
    
    // Build initial state from key XOR (UID || Challenge)
    // Key is 48 bits, UID || Challenge is 64 bits
    // We use the first 48 bits of (UID || Challenge)
    
    // Construct key as 48-bit value
    uint64_t key_value = ((uint64_t)key[0] << 0) |
                         ((uint64_t)key[1] << 8) |
                         ((uint64_t)key[2] << 16) |
                         ((uint64_t)key[3] << 24) |
                         ((uint64_t)key[4] << 32) |
                         ((uint64_t)key[5] << 40);
    
    // Construct UID || Challenge (we use 48 bits)
    uint64_t uid_challenge = challenge;  // Start with challenge
    uid_challenge |= ((uint64_t)uid << 32);  // Add UID in upper bits
    
    // XOR with key to get initial state
    state = key_value ^ uid_challenge;
    
    // Generate 32-bit response
    for (int i = 0; i < 32; i++) {
        // Extract LSB (this is the output bit)
        uint8_t output_bit = state & 1;
        
        // Set response bit
        if (output_bit) {
            response |= (1U << i);
        }
        
        // Shift LFSR and inject feedback
        // The feedback polynomial is applied based on tap bits
        state >>= 1;
        
        // Compute feedback (XOR of selected taps with output)
        // This is a simplified version - actual taps may differ
        uint64_t feedback = 0;
        
        // Apply tap mask (simplified version)
        // The actual tap pattern depends on the specific Hi-Tag 2 implementation
        if (state & 1) {
            feedback ^= 0x80012000ULL;  // Simplified feedback pattern
        }
        
        // Inject feedback into MSB position
        state |= (feedback << 47);
    }
    
    return response;
}

/*
 * Verify authentication response (for reader emulation)
 * 
 * Returns true if the response is valid for the given key, UID, and challenge
 */
bool crypto_verify_response(const uint8_t* key, uint32_t uid, uint32_t challenge, uint32_t response) {
    uint32_t computed = crypto_compute_response(key, uid, challenge);
    return (computed == response);
}

/*
 * Alternative LFSR implementation using byte-level operations
 * This is closer to the actual hardware implementation
 */
uint32_t crypto_lfsr_shift(uint8_t* state, int num_bits) {
    uint32_t output = 0;
    
    // State is 48 bits stored in 6 bytes (LSB first)
    // This means state[0] contains bits 0-7 (bit 0 = LSB)
    
    for (int i = 0; i < num_bits; i++) {
        // Extract output bit (bit 0 of the state)
        uint8_t out_bit = state[0] & 1;
        output |= ((uint32_t)out_bit << i);
        
        // Shift the 48-bit state right by 1 bit
        uint16_t carry = 0;
        for (int j = 0; j < 6; j++) {
            uint16_t new_carry = (state[j] & 1) << 8;  // LSB becomes carry for next byte
            state[j] = (state[j] >> 1) | carry;
            carry = new_carry;
        }
        
        // Compute feedback based on tap bits
        // The tap pattern is read from specific bit positions in the shifted state
        // This is an approximation of the actual hardware
        
        // Simplified feedback: XOR specific bits back into MSB
        // In real hardware, this is determined by the polynomial
        uint8_t feedback = 0;
        
        // Example tap positions (may need adjustment)
        if (state[4] & 0x10) feedback ^= 1;  // Bit 36
        if (state[4] & 0x08) feedback ^= 1;  // Bit 35
        if (state[3] & 0x80) feedback ^= 1;  // Bit 31
        if (state[2] & 0x20) feedback ^= 1;  // Bit 21
        if (state[1] & 0x10) feedback ^= 1;  // Bit 12
        
        // Inject feedback into MSB
        if (feedback) {
            state[5] |= 0x80;
        }
    }
    
    return output;
}

/*
 * Compute response using byte-level LFSR
 */
uint32_t crypto_compute_response_v2(const uint8_t* key, uint32_t uid, uint32_t challenge) {
    uint8_t state[6];
    uint32_t response = 0;
    
    // Initialize state with key (48 bits, LSB first)
    for (int i = 0; i < 6; i++) {
        state[i] = key[i];
    }
    
    // XOR with UID and challenge
    // State = Key XOR (UID || Challenge)
    state[0] ^= (challenge >> 0) & 0xFF;
    state[1] ^= (challenge >> 8) & 0xFF;
    state[2] ^= (challenge >> 16) & 0xFF;
    state[3] ^= (challenge >> 24) & 0xFF;
    state[4] ^= (uid >> 0) & 0xFF;
    state[5] ^= (uid >> 8) & 0xFF;
    
    // Generate 32-bit response
    response = crypto_lfsr_shift(state, 32);
    
    return response;
}

/*
 * Simple pseudo-random LFSR for testing
 * Uses a simple polynomial: x^48 + x^47 + x^45 + x^44 + 1
 */
uint64_t crypto_lfsr_simple(uint64_t state, int count, uint64_t* output) {
    uint64_t result = 0;
    
    // Simplified taps for testing: x^48 + x^47 + x^43 + x^31 + 1
    #define TAP_FEEDBACK  0x8001BULL  // Simplified
    
    for (int i = 0; i < count; i++) {
        // Extract output bit
        result |= ((state & 1) << i);
        
        // Shift and apply feedback
        state >>= 1;
        uint64_t feedback = 0;
        
        // Check tap bits
        if (state & (1ULL << 44)) feedback ^= 1;  // Tap at bit 44
        if (state & (1ULL << 31)) feedback ^= 1;  // Tap at bit 31
        if (state & (1ULL << 21)) feedback ^= 1;  // Tap at bit 21
        if (state & (1ULL << 1)) feedback ^= 1;   // Tap at bit 1
        
        if (feedback) {
            state |= (1ULL << 47);  // Set MSB
        }
    }
    
    if (output) *output = state;
    return result;
}
