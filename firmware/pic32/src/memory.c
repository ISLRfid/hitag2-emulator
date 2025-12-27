/*
 * Hi-Tag 2 Emulator - Memory Management Module
 * Manages the 256-bit tag memory (8 pages × 32 bits)
 */

#include "memory.h"
#include "debug.h"

// Memory pages (8 × 32 bits = 256 bits)
static page_t g_pages[NUM_PAGES];

// Default configuration for Paxton NET2
#define DEFAULT_CONFIG  0x00000000
#define DEFAULT_KEY     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

// Read-only pages (factory programmed)
static const bool g_page_writable[NUM_PAGES] = {
    false,  // Page 0: UID (read-only)
    true,   // Page 1: Configuration
    false,  // Page 2: Key bits 0-31 (read-only)
    false,  // Page 3: Key bits 32-47 (read-only)
    true,   // Page 4: User data
    true,   // Page 5: User data
    true,   // Page 6: User data
    true    // Page 7: User data
};

/*
 * Initialize memory subsystem
 */
void memory_init(void) {
    DEBUG_PRINT("Initializing memory subsystem...\r\n");
    
    // Initialize all pages to zero
    for (int i = 0; i < NUM_PAGES; i++) {
        g_pages[i].data = 0;
        g_pages[i].writable = g_page_writable[i];
    }
    
    DEBUG_PRINT("Memory initialized: %d pages × %d bits\r\n", NUM_PAGES, PAGE_SIZE);
}

/*
 * Load token data from buffer
 * Buffer format: page0, page1, page2, page3, page4, page5, page6, page7 (32 bits each)
 */
void memory_load_token(const uint8_t* buffer, uint16_t len) {
    if (len < NUM_PAGES * 4) {
        DEBUG_PRINT("ERROR: Buffer too small for token (%d < %d)\r\n", len, NUM_PAGES * 4);
        return;
    }
    
    for (int i = 0; i < NUM_PAGES; i++) {
        // Each page is 4 bytes (32 bits), little-endian
        g_pages[i].data = ((uint32_t)buffer[i * 4 + 0] << 0) |
                          ((uint32_t)buffer[i * 4 + 1] << 8) |
                          ((uint32_t)buffer[i * 4 + 2] << 16) |
                          ((uint32_t)buffer[i * 4 + 3] << 24);
    }
    
    DEBUG_PRINT("Token loaded: UID=%08X\r\n", g_pages[0].data);
}

/*
 * Save token data to buffer
 */
void memory_save_token(uint8_t* buffer, uint16_t* len) {
    for (int i = 0; i < NUM_PAGES; i++) {
        buffer[i * 4 + 0] = (g_pages[i].data >> 0) & 0xFF;
        buffer[i * 4 + 1] = (g_pages[i].data >> 8) & 0xFF;
        buffer[i * 4 + 2] = (g_pages[i].data >> 16) & 0xFF;
        buffer[i * 4 + 3] = (g_pages[i].data >> 24) & 0xFF;
    }
    
    if (len) {
        *len = NUM_PAGES * 4;
    }
}

/*
 * Read a page (32 bits)
 * page: page number (0-7)
 * returns: 32-bit page data
 */
uint32_t memory_read_page(uint8_t page) {
    if (page >= NUM_PAGES) {
        DEBUG_PRINT("ERROR: Invalid page %d\r\n", page);
        return 0;
    }
    
    return g_pages[page].data;
}

/*
 * Write a page (32 bits)
 * page: page number (0-7)
 * data: 32-bit data to write
 * returns: true if successful
 */
bool memory_write_page(uint8_t page, uint32_t data) {
    if (page >= NUM_PAGES) {
        DEBUG_PRINT("ERROR: Invalid page %d\r\n", page);
        return false;
    }
    
    if (!g_pages[page].writable) {
        DEBUG_PRINT("ERROR: Page %d is read-only\r\n", page);
        return false;
    }
    
    g_pages[page].data = data;
    return true;
}

/*
 * Get UID (Page 0)
 */
uint32_t memory_get_uid(void) {
    return g_pages[0].data;
}

/*
 * Set UID (Page 0, factory programmed)
 * Note: In real tags this is read-only, but emulator allows setting
 */
void memory_set_uid(uint32_t uid) {
    g_pages[0].data = uid;
}

/*
 * Get configuration (Page 1)
 */
uint32_t memory_get_config(void) {
    return g_pages[1].data;
}

/*
 * Set configuration (Page 1)
 */
void memory_set_config(uint32_t config) {
    g_pages[1].data = config;
}

/*
 * Get key (Pages 2-3)
 */
void memory_get_key(uint8_t* key) {
    // Page 2: Key bits 0-31
    uint32_t key_low = g_pages[2].data;
    
    // Page 3: Key bits 32-47 (upper 16 bits) + Password (lower 16 bits)
    uint32_t key_high = g_pages[3].data;
    
    // Extract key from page 3 (upper 16 bits)
    key[0] = (key_low >> 0) & 0xFF;
    key[1] = (key_low >> 8) & 0xFF;
    key[2] = (key_low >> 16) & 0xFF;
    key[3] = (key_low >> 24) & 0xFF;
    key[4] = (key_high >> 16) & 0xFF;  // Bits 32-39
    key[5] = (key_high >> 24) & 0xFF;  // Bits 40-47
}

/*
 * Set key (Pages 2-3)
 */
void memory_set_key(const uint8_t* key) {
    // Page 2: Key bits 0-31
    g_pages[2].data = ((uint32_t)key[0] << 0) |
                      ((uint32_t)key[1] << 8) |
                      ((uint32_t)key[2] << 16) |
                      ((uint32_t)key[3] << 24);
    
    // Page 3: Key bits 32-47 (upper 16 bits) + Password (lower 16 bits)
    // Keep password (lower 16 bits) if it exists
    uint16_t password = g_pages[3].data & 0xFFFF;
    g_pages[3].data = password |
                      ((uint32_t)key[4] << 16) |
                      ((uint32_t)key[5] << 24);
}

/*
 * Get user data page (Pages 4-7)
 */
uint32_t memory_get_user_page(uint8_t page) {
    if (page < 4 || page >= NUM_PAGES) {
        return 0;
    }
    return g_pages[page].data;
}

/*
 * Set user data page (Pages 4-7)
 */
void memory_set_user_page(uint8_t page, uint32_t data) {
    if (page < 4 || page >= NUM_PAGES) {
        return;
    }
    g_pages[page].data = data;
}

/*
 * Check if page is writable
 */
bool memory_is_page_writable(uint8_t page) {
    if (page >= NUM_PAGES) {
        return false;
    }
    return g_pages[page].writable;
}

/*
 * Get page lock status (from configuration)
 */
bool memory_is_locked(void) {
    // Check bit 1 of page 1 configuration
    return (g_pages[1].data >> 2) & 1;
}

/*
 * Get authentication requirement (from configuration)
 */
bool memory_auth_required(void) {
    // Check bit 0 of page 1 configuration
    return (g_pages[1].data >> 1) & 1;
}

/*
 * Clear all memory (reset to zeros)
 */
void memory_clear(void) {
    for (int i = 0; i < NUM_PAGES; i++) {
        g_pages[i].data = 0;
    }
}

/*
 * Load a predefined Paxton NET2 token
 */
void memory_load_paxton_demo(void) {
    // Example Paxton NET2 token data
    // Page 0: UID
    g_pages[0].data = 0x12345678;
    
    // Page 1: Configuration
    g_pages[1].data = 0x00000000;
    
    // Page 2-3: Authentication key
    g_pages[2].data = 0xA5A5A5A5;  // Key bits 0-31
    g_pages[3].data = 0x5A5A0000;  // Key bits 32-47 + password
    
    // Page 4-7: User data (Paxton NET2 format)
    g_pages[4].data = 0x00000001;  // Site code
    g_pages[5].data = 0x00001234;  // User ID
    g_pages[6].data = 0x00000000;  // Additional data
    g_pages[7].data = 0x00000000;  // Reserved
    
    DEBUG_PRINT("Paxton demo token loaded: UID=%08X\r\n", g_pages[0].data);
}

/*
 * Load a default test token
 */
void memory_load_default_token(void) {
    g_pages[0].data = 0xDEADBEEF;
    g_pages[1].data = 0x00000000;
    g_pages[2].data = 0x12345678;
    g_pages[3].data = 0x9ABC0000;
    g_pages[4].data = 0x00000000;
    g_pages[5].data = 0x00000000;
    g_pages[6].data = 0x00000000;
    g_pages[7].data = 0x00000000;
    
    DEBUG_PRINT("Default token loaded: UID=%08X\r\n", g_pages[0].data);
}

/*
 * Dump memory to debug output
 */
void memory_dump(void) {
    DEBUG_PRINT("=== Memory Dump ===\r\n");
    for (int i = 0; i < NUM_PAGES; i++) {
        DEBUG_PRINT("Page %d: %08X %s\r\n", 
            i, g_pages[i].data, 
            g_pages[i].writable ? "(R/W)" : "(R/O)");
    }
    DEBUG_PRINT("==================\r\n");
}
