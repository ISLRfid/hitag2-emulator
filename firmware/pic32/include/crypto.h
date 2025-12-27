/*
 * Hi-Tag 2 Emulator - Crypto Module Header
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stdbool.h>

// 48-bit key type
typedef struct {
    uint8_t bytes[6];
} hitag2_key_t;

// Initialize crypto subsystem
void crypto_init(void);

// Set/get secret key
void crypto_set_key(const uint8_t* key);
void crypto_get_key(uint8_t* key);

// Compute authentication response
// key: 48-bit secret key
// uid: 32-bit tag identifier
// challenge: 32-bit reader challenge
// returns: 32-bit encrypted response
uint32_t crypto_compute_response(const uint8_t* key, uint32_t uid, uint32_t challenge);

// Verify a response (for reader emulation)
bool crypto_verify_response(const uint8_t* key, uint32_t uid, uint32_t challenge, uint32_t response);

// Alternative implementation using byte-level operations
uint32_t crypto_compute_response_v2(const uint8_t* key, uint32_t uid, uint32_t challenge);

#endif // CRYPTO_H
