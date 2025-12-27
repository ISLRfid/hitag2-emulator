/*
 * Hi-Tag 2 Emulator - Debug Module
 * Simple debug output via UART (for development/debugging)
 */

#include "debug.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef ENABLE_DEBUG

// Debug UART configuration
#define DEBUG_BAUD 115200

/*
 * Initialize debug output
 */
void debug_init(void) {
    // UART1 for debug output
    // TX on RF_LED pin (RB11) for now, or use separate UART
    // For production, this would be disabled or use a different pin
    
    // Configure UART1 for debug
    // This is a placeholder - actual implementation depends on hardware
    DEBUG_PRINT("Debug output initialized\r\n");
}

/*
 * Print formatted string
 */
void debug_printf(const char* format, ...) {
    va_list args;
    char buffer[128];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Send buffer to UART
    debug_send_string(buffer);
}

/*
 * Send string to debug output
 */
void debug_send_string(const char* str) {
    while (*str) {
        debug_send_char(*str++);
    }
}

/*
 * Send single character
 */
void debug_send_char(char c) {
    // Placeholder - would send to UART TX pin
    // For now, we could use GPIO toggle for debugging
}

/*
 * Dump buffer in hex format
 */
void debug_dump_buffer(const uint8_t* buffer, uint16_t len) {
    DEBUG_PRINT("Buffer dump (%d bytes):\r\n", len);
    
    for (uint16_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            DEBUG_PRINT("%04X: ", i);
        }
        DEBUG_PRINT("%02X ", buffer[i]);
        
        if (i % 16 == 15) {
            DEBUG_PRINT("\r\n");
        }
    }
    
    if (len % 16 != 0) {
        DEBUG_PRINT("\r\n");
    }
}

/*
 * Print binary representation of value
 */
void debug_print_bits(uint32_t value, uint8_t num_bits) {
    char buffer[33];
    int i;
    
    for (i = 0; i < num_bits && i < 32; i++) {
        buffer[i] = (value & (1 << (num_bits - 1 - i))) ? '1' : '0';
    }
    buffer[i] = '\0';
    
    DEBUG_PRINT("0b%s\r\n", buffer);
}

#else

// Empty implementations when debug disabled
void debug_init(void) {}
void debug_printf(const char* format, ...) {}
void debug_send_string(const char* str) {}
void debug_send_char(char c) {}
void debug_dump_buffer(const uint8_t* buffer, uint16_t len) {}
void debug_print_bits(uint32_t value, uint8_t num_bits) {}

#endif // ENABLE_DEBUG
