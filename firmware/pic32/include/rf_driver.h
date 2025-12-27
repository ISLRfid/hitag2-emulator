/*
 * Hi-Tag 2 Emulator - RF Driver Header
 */

#ifndef RF_DRIVER_H
#define RF_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// RF State Machine States
typedef enum {
    RF_STATE_IDLE = 0,
    RF_STATE_LISTENING,
    RF_STATE_PROCESSING,
    RF_STATE_TRANSMITTING,
    RF_STATE_ERROR,
    RF_STATE_HALT
} rf_state_t;

// RF Configuration
typedef struct {
    uint32_t carrier_freq;    // 125000 Hz
    uint32_t bit_rate;        // 4000 bps
    uint16_t bit_period;      // 250 µs
    uint16_t half_bit;        // 125 µs
    uint16_t gap_time;        // 256 µs
    uint16_t response_delay;  // 256 µs
} rf_config_t;

// Initialize RF subsystem
void rf_driver_init(void);

// Carrier control
void rf_carrier_on(void);
void rf_carrier_off(void);

// Modulation
void rf_send_manchester(const uint8_t* data, uint16_t num_bits);
void rf_send_bpsk(const uint8_t* data, uint16_t num_bits);

// Demodulation
uint16_t rf_receive_manchester(uint8_t* buffer, uint16_t max_bits, uint32_t timeout_ms);
uint16_t rf_receive_simple(uint8_t* buffer, uint16_t max_bits, uint32_t timeout_ms);

// Field detection
bool rf_wait_for_field(uint32_t timeout_ms);
bool rf_get_field_detected(void);

// State management
rf_state_t rf_get_state(void);
void rf_set_state(rf_state_t state);

// Process loop
void rf_driver_process(void);

// Timer tick
void rf_driver_tick(void);

// PWM control
void pwm_init(void);
void pwm_set_duty(uint16_t duty);

// Timing helpers
void rf_send_start_gap(uint16_t gap_us);
void rf_send_response_delay(uint16_t delay_us);

#endif // RF_DRIVER_H
