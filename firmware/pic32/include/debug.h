/*
 * Hi-Tag 2 Emulator - Debug Module Header
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

// Enable/disable debug output
// #define ENABLE_DEBUG

// Debug print macro
#ifdef ENABLE_DEBUG
#define DEBUG_PRINT(fmt, ...) \
    debug_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) 
#endif

// Initialize debug subsystem
void debug_init(void);

// Print formatted string
void debug_printf(const char* format, ...);

// Send string
void debug_send_string(const char* str);
void debug_send_char(char c);

// Buffer dump
void debug_dump_buffer(const uint8_t* buffer, uint16_t len);

// Binary output
void debug_print_bits(uint32_t value, uint8_t num_bits);

#endif // DEBUG_H
