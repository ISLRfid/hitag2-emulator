/*
 * Hi-Tag 2 Emulator - Memory Module Header
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

// Memory constants
#define NUM_PAGES     8
#define PAGE_SIZE     32  // bits per page
#define TOTAL_BITS    256 // 8 pages × 32 bits

// Memory page structure
typedef struct {
    uint32_t data;
    bool writable;
} page_t;

// Initialize memory subsystem
void memory_init(void);

// Token data management
void memory_load_token(const uint8_t* buffer, uint16_t len);
void memory_save_token(uint8_t* buffer, uint16_t* len);

// Page access
uint32_t memory_read_page(uint8_t page);
bool memory_write_page(uint8_t page, uint32_t data);

// UID access
uint32_t memory_get_uid(void);
void memory_set_uid(uint32_t uid);

// Configuration access
uint32_t memory_get_config(void);
void memory_set_config(uint32_t config);

// Key access
void memory_get_key(uint8_t* key);
void memory_set_key(const uint8_t* key);

// User data access
uint32_t memory_get_user_page(uint8_t page);
void memory_set_user_page(uint8_t page, uint32_t data);

// Status queries
bool memory_is_page_writable(uint8_t page);
bool memory_is_locked(void);
bool memory_auth_required(void);

// Utility functions
void memory_clear(void);
void memory_load_paxton_demo(void);
void memory_load_default_token(void);
void memory_dump(void);

#endif // MEMORY_H
