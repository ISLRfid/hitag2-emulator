/*
 * Hi-Tag 2 Emulator - PIC32 Firmware
 * Main Header File
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>

// Application modes
typedef enum {
    MODE_IDLE = 0,
    MODE_EMULATION,
    MODE_DEBUG
} app_mode_t;

// Application state structure
typedef struct {
    app_mode_t mode;
    rf_state_t rf_state;
    bool token_loaded;
    bool debug_enabled;
} app_state_t;

// External application state
extern app_state_t g_app_state;

// Function prototypes
void system_init(void);
void osc_configure(void);
void gpio_init(void);
void timer_init(void);
uint32_t system_get_ticks(void);
void system_delay_ms(uint32_t ms);
void system_delay_us(uint32_t us);
void status_led_update(void);

#endif // MAIN_H
