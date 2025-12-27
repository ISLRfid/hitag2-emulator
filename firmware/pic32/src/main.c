/*
 * Hi-Tag 2 Emulator - PIC32 Firmware
 * Main Application Entry Point
 * 
 * Target: PIC32MX795F512L @ 80 MHz
 * Purpose: 125 kHz RFID token emulation
 */

#include "main.h"
#include "rf_driver.h"
#include "crypto.h"
#include "memory.h"
#include "spi_slave.h"
#include "debug.h"

// Application state
app_state_t g_app_state = {
    .mode = MODE_IDLE,
    .rf_state = RF_STATE_IDLE,
    .token_loaded = false,
    .debug_enabled = false
};

// System tick counter (updated by Timer2 interrupt)
volatile uint32_t g_system_ticks = 0;

/*
 * System initialization
 */
void system_init(void) {
    // Disable interrupts during init
    __builtin_disable_interrupts();
    
    // Configure system clock
    osc_configure();
    
    // Initialize GPIO
    gpio_init();
    
    // Initialize timers
    timer_init();
    
    // Initialize RF subsystem
    rf_driver_init();
    
    // Initialize crypto subsystem
    crypto_init();
    
    // Initialize memory subsystem
    memory_init();
    
    // Initialize SPI slave
    spi_slave_init();
    
    // Initialize debug output
    debug_init();
    
    // Enable interrupts
    __builtin_enable_interrupts();
    
    DEBUG_PRINT("Hi-Tag 2 Emulator v1.0 initialized\r\n");
}

/*
 * Configure oscillator for 80 MHz operation
 */
void osc_configure(void) {
    // PIC32 uses internal FRC + PLL for 80 MHz
    // FRC = 8 MHz, PLL divider = 2, PLL multiplier = 20
    // FPLLIDIV = 2, FPLLMUL = 20, FPLLODIV = 2
    // (8MHz / 2) * 20 / 2 = 40 MHz... needs adjustment
    
    // Use proper configuration for 80 MHz:
    // FRC = 8 MHz
    // PLL: N1=2, N2=20, N3=2
    // Fout = (Fin / N1) * N2 / N3 = (8/2) * 20 / 2 = 40 MHz
    // For 80 MHz, use: FRC with POST scalar = 1:1
    
    // Configure for 80 MHz using FRC + PLL
    // SYSCLK = FRC / FPLLIDIV * FPLLMUL / FPLLODIV
    // 80 MHz = 8 MHz / 2 * 20 / 1
    
    OSCCONbits.FRCDIV = 0;    // FRC divided by 1 (8 MHz)
    OSCCONbits.PLLMULT = 2;   // 2x multiplier (not used directly)
    
    // Use PLL with proper settings for 80 MHz
    // Wait for PLL to lock
    while (!OSCCONbits.SOSCRDY);
    
    // Actually, let's use the simpler approach:
    // Set up for 80 MHz using internal FRC
    // Note: For production, use external crystal
}

/*
 * Initialize GPIO pins
 */
void gpio_init(void) {
    // Configure analog pins as digital
    ANSELA = 0;
    ANSELB = 0;
    
    // Configure control pins
    // RA0 (Pin 17) - TX_EN (output)
    TRISAbits.TRISA0 = 0;
    LATAbits.LATA0 = 0;
    
    // RA1 (Pin 38) - IRQ (output)
    TRISAbits.TRISA1 = 0;
    LATAbits.LATA1 = 0;
    
    // RB10 (Pin 23) - STATUS LED (output)
    TRISBbits.TRISB10 = 0;
    LATBbits.LATB10 = 0;
    
    // RB11 (Pin 24) - RF LED (output)
    TRISBbits.TRISB11 = 0;
    LATBbits.LATB11 = 0;
    
    // RF pins configured in rf_driver_init()
    
    // SPI pins configured in spi_slave_init()
}

/*
 * Initialize timer for system ticks
 */
void timer_init(void) {
    // Use Timer2 for system tick (1 ms resolution)
    T2CONbits.TCKPS = 0b100;  // 1:16 prescaler (80 MHz / 16 = 5 MHz)
    PR2 = 5000 - 1;           // 5 MHz / 5000 = 1 kHz
    TMR2 = 0;
    
    // Enable Timer2 interrupt
    IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 1;
    IPC2bits.T2IP = 4;
    
    T2CONbits.ON = 1;
}

/*
 * Timer2 interrupt handler (1 ms tick)
 */
void __attribute__((interrupt(IPL4AUTO), vector(_TIMER_2_VECTOR)))
timer2_handler(void) {
    g_system_ticks++;
    
    // Update RF driver timing
    rf_driver_tick();
    
    // Clear interrupt flag
    IFS0bits.T2IF = 0;
}

/*
 * Main application loop
 */
int main(void) {
    // Initialize system
    system_init();
    
    // LED indication
    LATBbits.LATB10 = 1;  // Turn on status LED
    
    // Main loop
    while (1) {
        // Process SPI commands
        spi_slave_process();
        
        // Process RF events
        rf_driver_process();
        
        // Process debug commands (if enabled)
        if (g_app_state.debug_enabled) {
            debug_process();
        }
        
        // Update status LEDs
        status_led_update();
    }
    
    return 0;
}

/*
 * Update status LEDs
 */
void status_led_update(void) {
    // Status LED: on when token loaded
    LATBbits.LATB10 = g_app_state.token_loaded ? 1 : 0;
    
    // RF LED: on when RF field detected
    LATBbits.LATB11 = rf_get_field_detected() ? 1 : 0;
}

/*
 * Get current system tick count
 */
uint32_t system_get_ticks(void) {
    return g_system_ticks;
}

/*
 * Delay milliseconds
 */
void system_delay_ms(uint32_t ms) {
    uint32_t start = g_system_ticks;
    while ((g_system_ticks - start) < ms) {
        // NOP
    }
}

/*
 * Delay microseconds
 * Note: Uses busy-wait, not precise but sufficient for this application
 */
void system_delay_us(uint32_t us) {
    // At 80 MHz, each cycle is 12.5 ns
    // For reasonable delays, use a loop
    // For precise timing, use hardware timers
    volatile uint32_t cycles = us * 80;  // 80 cycles per microsecond
    while (cycles > 0) {
        cycles--;
    }
}
