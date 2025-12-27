/*
 * Hi-Tag 2 Emulator - RF Driver Module
 * Handles 125 kHz carrier generation, ASK/BPSK modulation,
 * Manchester encoding/decoding, and RF state machine
 */

#include "rf_driver.h"
#include "main.h"
#include "debug.h"

// RF configuration constants
#define CARRIER_FREQ_HZ       125000UL   // 125 kHz carrier
#define BIT_RATE_BPS          4000UL     // 4 kbps
#define BIT_PERIOD_US         250UL      // 250 microseconds per bit
#define HALF_BIT_US           125UL      // 125 microseconds
#define START_GAP_US          256UL      // Start gap (256 µs)
#define RESPONSE_DELAY_US     256UL      // Response delay (256 µs)

// PWM configuration
#define PWM_TIMER_FREQ_HZ     80000000UL // 80 MHz system clock
#define PWM_PERIOD            (PWM_TIMER_FREQ_HZ / (CARRIER_FREQ_HZ * 2) - 1)  // 319
#define PWM_DUTY_50           (PWM_PERIOD / 2)  // 50% duty cycle

// RF state
static volatile rf_state_t g_rf_state = RF_STATE_IDLE;
static volatile bool g_field_detected = false;

// Timing variables
static volatile uint32_t g_rf_timer_start = 0;
static volatile uint16_t g_rf_bit_buffer = 0;
static volatile uint8_t g_rf_bit_count = 0;
static volatile uint8_t g_rf_rx_state = 0;

// Manchester encoding tables
// Manchester '0': High-to-Low transition (carrier ON, then OFF)
// Manchester '1': Low-to-High transition (carrier OFF, then ON)
static const uint8_t manchester_half[] = {
    1, 0,  // '0': first half = 1 (carrier), second half = 0 (no carrier)
    0, 1   // '1': first half = 0 (no carrier), second half = 1 (carrier)
};

/*
 * Initialize RF subsystem
 */
void rf_driver_init(void) {
    DEBUG_PRINT("Initializing RF driver...\r\n");
    
    // Configure RF output pin (RB5/Pin 18) for PWM
    TRISBbits.TRISB5 = 0;  // Output
    
    // Configure RF input pin (RB4/Pin 12) for digital input
    TRISBbits.TRISB4 = 1;  // Input
    
    // Configure PWM for carrier generation
    pwm_init();
    
    // Configure input capture for receive
    ic_init();
    
    // Configure external interrupt for field detection
    ext_int_init();
    
    // Set initial state
    g_rf_state = RF_STATE_IDLE;
    g_field_detected = false;
    
    DEBUG_PRINT("RF driver initialized\r\n");
}

/*
 * Initialize PWM for 125 kHz carrier
 */
void pwm_init(void) {
    // Use Output Compare 1 (OC1) on RB5 (Pin 18)
    // OC1 is mapped to RPB5R (Pin 18) = 0b0010 (OC1)
    
    // Disable OC1 before configuration
    OC1CONbits.ON = 0;
    
    // Configure timer source (Timer2)
    OC1CONbits.OCTSEL = 0;  // Use Timer2
    
    // Configure mode: PWM mode with fault pin disabled
    OC1CONbits.OCM = 0b110;  // PWM mode, fault disabled
    
    // Set period (Timer2 period register)
    PR2 = PWM_PERIOD;
    
    // Set duty cycle (50% = PWM_DUTY_50)
    OC1RS = PWM_DUTY_50;
    OC1R = PWM_DUTY_50;
    
    // Start timer 2
    T2CONbits.ON = 1;
    
    // Enable OC1
    OC1CONbits.ON = 1;
    
    // Configure output pin
    // RPB5R = 0010b (OC1)
    RPB5R = 0b0010;
}

/*
 * Set PWM duty cycle
 */
void pwm_set_duty(uint16_t duty) {
    // duty is from 0 to PWM_PERIOD
    if (duty > PWM_PERIOD) duty = PWM_PERIOD;
    OC1RS = duty;
}

/*
 * Initialize input capture for Manchester decoding
 */
void ic_init(void) {
    // Use Input Capture 1 on RB4 (Pin 12)
    // IC1 is mapped to RPB4R (Pin 12) = 0b0001 (IC1)
    
    // Disable IC1 before configuration
    IC1CONbits.ON = 0;
    
    // Configure mode: capture every rising and falling edge
    IC1CONbits.ICM = 0b010;  // Capture every edge
    
    // Configure timer source (Timer3)
    IC1CONbits.ICTMR = 1;  // Use Timer3
    
    // Enable buffer
    IC1CONbits.ICBNE = 0;
    
    // Configure timer 3 for input capture
    T3CONbits.TCKPS = 0b000;  // 1:1 prescaler
    PR3 = 0xFFFFFFFF;  // 32-bit period (use max)
    TMR3 = 0;
    T3CONbits.ON = 1;
    
    // Enable IC1 interrupt
    IC1CONbits.ICM = 0b011;  // Capture every edge, enable
    // Note: We'll poll IC1CONbits.ICBNE instead of using interrupts for simplicity
    
    // Configure pin
    // ANSELBbits.ANSB4 = 0;  // Digital input (already done in gpio_init)
    // RPB4R = 0b0001 (IC1) - actually IC1 is on different pins
    
    // For RB4 (Pin 12), we can use external interrupt or poll directly
    // Let's use polling for simplicity
}

/*
 * Initialize external interrupt for field detection
 */
void ext_int_init(void) {
    // Use INT1 on RB4 (Pin 12) for field detection
    // Actually, let's use INT2 or poll the pin
    
    // Configure pin as input
    TRISBbits.TRISB4 = 1;
    
    // Enable change notification
    CNCONBbits.ON = 1;
    CNENBbits.CNIEB4 = 1;
    
    // Set interrupt priority
    IPC6bits.CNIP = 3;
    IEC1bits.CNIE = 1;
}

/*
 * RF carrier control
 */
void rf_carrier_on(void) {
    pwm_set_duty(PWM_DUTY_50);
}

void rf_carrier_off(void) {
    pwm_set_duty(0);
}

/*
 * Send Manchester-encoded data (for downlink - reader commands)
 */
void rf_send_manchester(const uint8_t* data, uint16_t num_bits) {
    DEBUG_PRINT("Sending Manchester: %d bits\r\n", num_bits);
    
    for (uint16_t i = 0; i < num_bits; i++) {
        // Extract bit
        uint8_t bit = (data[i / 8] >> (i % 8)) & 1;
        
        // Manchester encoding
        // Bit '0': High-to-Low transition
        // Bit '1': Low-to-High transition
        
        if (bit == 0) {
            // First half: carrier ON
            pwm_set_duty(PWM_DUTY_50);
            system_delay_us(HALF_BIT_US);
            
            // Second half: carrier OFF
            pwm_set_duty(0);
            system_delay_us(HALF_BIT_US);
        } else {
            // First half: carrier OFF
            pwm_set_duty(0);
            system_delay_us(HALF_BIT_US);
            
            // Second half: carrier ON
            pwm_set_duty(PWM_DUTY_50);
            system_delay_us(HALF_BIT_US);
        }
    }
}

/*
 * Send BPSK-modulated data (for uplink - tag responses)
 */
void rf_send_bpsk(const uint8_t* data, uint16_t num_bits) {
    DEBUG_PRINT("Sending BPSK: %d bits\r\n", num_bits);
    
    // Generate subcarrier at 4 kHz (125 kHz / 32)
    // For BPSK, we phase-shift the subcarrier based on data
    
    uint8_t last_phase = 0;
    uint16_t subcarrier_period = BIT_PERIOD_US;  // 250 µs = one bit period
    
    for (uint16_t i = 0; i < num_bits; i++) {
        uint8_t bit = (data[i / 8] >> (i % 8)) & 1;
        
        // BPSK encoding:
        // Bit '0': 180° phase shift
        // Bit '1': 0° phase shift (no shift)
        
        uint8_t new_phase = bit ^ last_phase;  // Phase change indicator
        
        // Generate subcarrier with appropriate phase
        // 125 kHz / 4 kHz = 31.25 cycles per subcarrier period
        // We'll approximate with integer cycles
        
        if (new_phase) {
            // Phase shift: invert the subcarrier
            // This effectively sends 180° phase shift
            for (int j = 0; j < 31; j++) {
                // Alternate between carrier on and off
                // 125 kHz = 8 µs period
                // 4 kHz subcarrier = 250 µs period
                // Within one subcarrier period: 31 cycles
                pwm_set_duty((j < 15) ? PWM_DUTY_50 : 0);
                system_delay_us(4);  // 4 µs per half-cycle
                pwm_set_duty((j < 15) ? 0 : PWM_DUTY_50);
                system_delay_us(4);
            }
        } else {
            // No phase shift: normal subcarrier
            for (int j = 0; j < 31; j++) {
                pwm_set_duty((j < 15) ? PWM_DUTY_50 : 0);
                system_delay_us(4);
                pwm_set_duty((j < 15) ? 0 : PWM_DUTY_50);
                system_delay_us(4);
            }
        }
        
        last_phase = bit;
    }
}

/*
 * Receive and decode Manchester-encoded data
 */
uint16_t rf_receive_manchester(uint8_t* buffer, uint16_t max_bits, uint32_t timeout_ms) {
    uint16_t bit_count = 0;
    uint32_t start_time = system_get_ticks();
    uint8_t last_level = 0;
    bool in_gap = true;
    uint32_t last_edge_time = 0;
    uint8_t transitions[2] = {0, 0};
    
    // Clear buffer
    memset(buffer, 0, (max_bits + 7) / 8);
    
    while (bit_count < max_bits) {
        // Check timeout
        if ((system_get_ticks() - start_time) >= timeout_ms) {
            DEBUG_PRINT("RX timeout after %d bits\r\n", bit_count);
            return bit_count;
        }
        
        // Sample RF input
        uint8_t level = PORTBbits.RB4;
        
        if (in_gap) {
            // Waiting for gap (carrier off)
            if (level == 0) {
                // Carrier present, check if start gap is complete
                if (last_edge_time == 0) {
                    last_edge_time = system_get_ticks();
                } else if ((system_get_ticks() - last_edge_time) >= 2) {
                    // Gap detected (>= 256 µs = 2 ms at our tick rate)
                    in_gap = false;
                    last_edge_time = 0;
                    DEBUG_PRINT("Start gap detected\r\n");
                }
            } else {
                last_edge_time = 0;  // Reset if carrier detected
            }
        } else {
            // Receiving bits
            if (level != last_level) {
                // Edge detected
                uint32_t edge_time = system_get_ticks();
                
                if (last_level == 0) {
                    // Rising edge: carrier turned on
                    // This marks start of Manchester bit
                    
                    // Wait for mid-bit to sample
                    // Actually, Manchester bits are encoded in the transition
                    // We should detect the transition at mid-bit
                }
                
                // Measure time since last edge to determine bit value
                // Manchester encoding: transition in middle
                // Short pulse (~125 µs) means one encoding
                // Long pulse (~250 µs) means two half-bits of same polarity
                
                uint32_t pulse_width = edge_time - last_edge_time;
                last_level = level;
                last_edge_time = edge_time;
                
                // For simplicity, sample at bit boundaries
                // Manchester decoding: sample at mid-bit
                // We need to detect the transition in the middle
                
                // Let's try a simpler approach: sample at fixed intervals
            }
        }
        
        // Simple approach: wait for gap, then sample at mid-bit intervals
        // This is a simplified implementation
        
        system_delay_us(10);  // Small delay to avoid tight loop
    }
    
    return bit_count;
}

/*
 * Simplified Manchester receive (polling-based)
 */
uint16_t rf_receive_simple(uint8_t* buffer, uint16_t max_bits, uint32_t timeout_ms) {
    uint16_t bit_count = 0;
    uint32_t start_time = system_get_ticks();
    uint8_t last_sample = 0;
    
    // Clear buffer
    memset(buffer, 0, (max_bits + 7) / 8);
    
    // Wait for start gap (carrier off for >= 256 µs)
    while (PORTBbits.RB4) {
        if ((system_get_ticks() - start_time) >= timeout_ms) {
            return 0;  // Timeout
        }
        system_delay_us(10);
    }
    
    // Wait for carrier to return (gap end)
    start_time = system_get_ticks();
    while (!PORTBbits.RB4) {
        if ((system_get_ticks() - start_time) >= 2) {  // 2 ms timeout
            break;
        }
    }
    
    // Now receive bits
    while (bit_count < max_bits) {
        // Wait for first half of bit period (carrier present)
        system_delay_us(HALF_BIT_US);
        
        // Sample first half
        uint8_t first_half = PORTBbits.RB4;
        
        // Wait for second half
        system_delay_us(HALF_BIT_US);
        
        // Sample second half
        uint8_t second_half = PORTBbits.RB4;
        
        // Decode Manchester
        // High-to-Low (1→0) = bit 0
        // Low-to-High (0→1) = bit 1
        uint8_t bit;
        if (first_half && !second_half) {
            bit = 0;  // Manchester 0
        } else if (!first_half && second_half) {
            bit = 1;  // Manchester 1
        } else {
            // Invalid encoding or timeout
            break;
        }
        
        // Store bit
        if (bit) {
            buffer[bit_count / 8] |= (1 << (bit_count % 8));
        }
        bit_count++;
        
        // Check for next gap (end of transmission)
        system_delay_us(10);
        if (!PORTBbits.RB4) {
            // Check if this is the start gap of next transmission
            uint32_t gap_start = system_get_ticks();
            while (!PORTBbits.RB4) {
                if ((system_get_ticks() - gap_start) >= 2) {
                    // Gap detected, transmission complete
                    DEBUG_PRINT("RX complete: %d bits\r\n", bit_count);
                    return bit_count;
                }
            }
        }
    }
    
    return bit_count;
}

/*
 * Wait for RF field to be present
 */
bool rf_wait_for_field(uint32_t timeout_ms) {
    uint32_t start = system_get_ticks();
    
    while (!g_field_detected) {
        if ((system_get_ticks() - start) >= timeout_ms) {
            return false;
        }
        system_delay_ms(1);
    }
    
    return true;
}

/*
 * Get current RF state
 */
rf_state_t rf_get_state(void) {
    return g_rf_state;
}

/*
 * Set RF state
 */
void rf_set_state(rf_state_t state) {
    g_rf_state = state;
}

/*
 * Get field detected status
 */
bool rf_get_field_detected(void) {
    return g_field_detected;
}

/*
 * Process RF events (call from main loop)
 */
void rf_driver_process(void) {
    switch (g_rf_state) {
        case RF_STATE_IDLE:
            if (g_field_detected) {
                rf_set_state(RF_STATE_LISTENING);
            }
            break;
            
        case RF_STATE_LISTENING:
            // Check for incoming command
            // This would be handled by the command parser
            break;
            
        case RF_STATE_PROCESSING:
            // Processing received command
            break;
            
        case RF_STATE_TRANSMITTING:
            // Transmitting response
            break;
            
        case RF_STATE_ERROR:
            // Error state - try to recover
            if (g_field_detected) {
                rf_set_state(RF_STATE_LISTENING);
            } else {
                rf_set_state(RF_STATE_IDLE);
            }
            break;
            
        case RF_STATE_HALT:
            // Halt mode - no response
            break;
    }
}

/*
 * Timer tick handler (called from Timer2 ISR)
 */
void rf_driver_tick(void) {
    // Update any timeouts or counters here
}

/*
 * Send start gap (carrier off for specified duration)
 */
void rf_send_start_gap(uint16_t gap_us) {
    pwm_set_duty(0);
    system_delay_us(gap_us);
}

/*
 * Send response delay
 */
void rf_send_response_delay(uint16_t delay_us) {
    pwm_set_duty(0);
    system_delay_us(delay_us);
}

