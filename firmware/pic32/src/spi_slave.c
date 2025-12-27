/*
 * Hi-Tag 2 Emulator - SPI Slave Module
 * Handles communication with Arduino master
 * 
 * SPI Configuration:
 * - Mode: SPI_MODE_0 (CPOL=0, CPHA=0)
 * - Clock: 4 MHz (for reliability)
 * - Data Order: MSB first
 * - Chip Select: Active low (SS1 on pin 69)
 */

#include "spi_slave.h"
#include "main.h"
#include "memory.h"
#include "crypto.h"
#include "rf_driver.h"
#include "debug.h"
#include <string.h>

// SPI command definitions
#define CMD_PING          0x01
#define CMD_RESET         0x02
#define CMD_READ_PAGE     0x10
#define CMD_WRITE_PAGE    0x20
#define CMD_SET_KEY       0x30
#define CMD_GET_KEY       0x31
#define CMD_SET_UID       0x40
#define CMD_GET_UID       0x41
#define CMD_SET_CONFIG    0x50
#define CMD_GET_CONFIG    0x51
#define CMD_LOAD_TOKEN    0x60
#define CMD_SAVE_TOKEN    0x61
#define CMD_START_EMULATE 0x70
#define CMD_STOP_EMULATE  0x71
#define CMD_GET_STATUS    0x80
#define CMD_DEBUG_MODE    0xA0

// Status codes
#define STATUS_OK         0x00
#define STATUS_ERR        0x01
#define STATUS_BUSY       0x02
#define STATUS_RDY        0x03

// SPI buffer sizes
#define SPI_RX_BUFFER_SIZE  64
#define SPI_TX_BUFFER_SIZE  64

// SPI buffers
static uint8_t g_spi_rx_buffer[SPI_RX_BUFFER_SIZE];
static uint8_t g_spi_tx_buffer[SPI_TX_BUFFER_SIZE];
static volatile uint8_t g_spi_rx_index = 0;
static volatile uint8_t g_spi_tx_index = 0;
static volatile bool g_spi_transfer_complete = false;

/*
 * Initialize SPI slave
 */
void spi_slave_init(void) {
    DEBUG_PRINT("Initializing SPI slave...\r\n");
    
    // Configure SPI pins
    // SDI1 (Pin 25) - MOSI from master
    TRISBbits.TRISB13 = 1;  // Input
    
    // SDO1 (Pin 72) - MISO to master
    TRISBbits.TRISB11 = 0;  // Output
    LATBbits.LATB11 = 0;
    
    // SCK1 (Pin 70) - Clock from master
    TRISBbits.TRISB9 = 1;  // Input
    
    // SS1 (Pin 69) - Chip select (active low)
    TRISBbits.TRISB8 = 1;  // Input
    
    // Configure SPI1 module
    SPI1CONbits.ON = 0;  // Disable during configuration
    
    // Mode 0: CPOL=0, CPHA=0
    SPI1CONbits.CKP = 0;
    SPI1CONbits.CKE = 1;  // Data changes on falling edge, captured on rising
    
    // 8-bit mode
    SPI1CONbits.MODE8 = 1;
    SPI1CONbits.MODE16 = 0;
    
    // Master-slave mode (slave)
    SPI1CONbits.MSTEN = 0;
    
    // Enable SS input (slave mode)
    SPI1CONbits.SSEN = 1;
    
    // Buffer settings
    SPI1CONbits.SPIFE = 0;  // Frame sync pulse not used
    
    // Set baud rate (master drives clock, but we set it anyway)
    SPI1BRG = 19;  // 80 MHz / (2 * (19 + 1)) = 2 MHz
    
    // Clear buffers
    memset(g_spi_rx_buffer, 0, SPI_RX_BUFFER_SIZE);
    memset(g_spi_tx_buffer, 0, SPI_TX_BUFFER_SIZE);
    g_spi_rx_index = 0;
    g_spi_tx_index = 0;
    
    // Enable SPI
    SPI1CONbits.ON = 1;
    
    // Clear any overflow
    uint8_t temp = SPI1BUF;
    (void)temp;
    
    // Enable SPI interrupt
    IEC0bits.SPI1EIE = 0;  // No error interrupt
    IEC0bits.SPI1RXIE = 1;  // RX interrupt enabled
    IEC0bits.SPI1TXIE = 0;  // TX interrupt disabled (we'll poll)
    
    DEBUG_PRINT("SPI slave initialized (Mode 0, 2 MHz)\r\n");
}

/*
 * Process SPI commands (call from main loop)
 */
void spi_slave_process(void) {
    // Check for received data
    if (g_spi_rx_index >= 2) {
        // Process command
        spi_process_command();
    }
}

/*
 * Process a single SPI command
 */
static void spi_process_command(void) {
    uint8_t cmd = g_spi_rx_buffer[0];
    uint8_t len = g_spi_rx_index;
    uint8_t status = STATUS_OK;
    
    // Clear buffers for response
    memset(g_spi_tx_buffer, 0, SPI_TX_BUFFER_SIZE);
    
    switch (cmd) {
        case CMD_PING:
            // Simple ping test
            g_spi_tx_buffer[0] = STATUS_OK;
            g_spi_tx_buffer[1] = 0x00;  // Pong
            spi_set_tx_length(2);
            DEBUG_PRINT("SPI: PING received\r\n");
            break;
            
        case CMD_RESET:
            // Reset PIC32 state
            memory_init();
            crypto_init();
            rf_set_state(RF_STATE_IDLE);
            g_app_state.mode = MODE_IDLE;
            g_app_state.token_loaded = false;
            g_spi_tx_buffer[0] = STATUS_OK;
            spi_set_tx_length(1);
            DEBUG_PRINT("SPI: RESET received\r\n");
            break;
            
        case CMD_READ_PAGE:
            if (len >= 2) {
                uint8_t page = g_spi_rx_buffer[1];
                uint32_t data = memory_read_page(page);
                g_spi_tx_buffer[0] = STATUS_OK;
                g_spi_tx_buffer[1] = (data >> 0) & 0xFF;
                g_spi_tx_buffer[2] = (data >> 8) & 0xFF;
                g_spi_tx_buffer[3] = (data >> 16) & 0xFF;
                g_spi_tx_buffer[4] = (data >> 24) & 0xFF;
                spi_set_tx_length(5);
                DEBUG_PRINT("SPI: READ_PAGE page=%d data=%08X\r\n", page, data);
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_WRITE_PAGE:
            if (len >= 6) {
                uint8_t page = g_spi_rx_buffer[1];
                uint32_t data = ((uint32_t)g_spi_rx_buffer[2] << 0) |
                                ((uint32_t)g_spi_rx_buffer[3] << 8) |
                                ((uint32_t)g_spi_rx_buffer[4] << 16) |
                                ((uint32_t)g_spi_rx_buffer[5] << 24);
                if (memory_write_page(page, data)) {
                    g_spi_tx_buffer[0] = STATUS_OK;
                } else {
                    g_spi_tx_buffer[0] = STATUS_ERR;
                }
                spi_set_tx_length(1);
                DEBUG_PRINT("SPI: WRITE_PAGE page=%d data=%08X\r\n", page, data);
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_SET_KEY:
            if (len >= 7) {
                uint8_t key[6];
                memcpy(key, &g_spi_rx_buffer[1], 6);
                crypto_set_key(key);
                memory_set_key(key);
                g_spi_tx_buffer[0] = STATUS_OK;
                spi_set_tx_length(1);
                DEBUG_PRINT("SPI: SET_KEY\r\n");
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_GET_KEY:
            {
                uint8_t key[6];
                crypto_get_key(key);
                g_spi_tx_buffer[0] = STATUS_OK;
                memcpy(&g_spi_tx_buffer[1], key, 6);
                spi_set_tx_length(7);
                DEBUG_PRINT("SPI: GET_KEY\r\n");
            }
            break;
            
        case CMD_SET_UID:
            if (len >= 5) {
                uint32_t uid = ((uint32_t)g_spi_rx_buffer[1] << 0) |
                               ((uint32_t)g_spi_rx_buffer[2] << 8) |
                               ((uint32_t)g_spi_rx_buffer[3] << 16) |
                               ((uint32_t)g_spi_rx_buffer[4] << 24);
                memory_set_uid(uid);
                g_spi_tx_buffer[0] = STATUS_OK;
                spi_set_tx_length(1);
                DEBUG_PRINT("SPI: SET_UID=%08X\r\n", uid);
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_GET_UID:
            {
                uint32_t uid = memory_get_uid();
                g_spi_tx_buffer[0] = STATUS_OK;
                g_spi_tx_buffer[1] = (uid >> 0) & 0xFF;
                g_spi_tx_buffer[2] = (uid >> 8) & 0xFF;
                g_spi_tx_buffer[3] = (uid >> 16) & 0xFF;
                g_spi_tx_buffer[4] = (uid >> 24) & 0xFF;
                spi_set_tx_length(5);
            }
            break;
            
        case CMD_SET_CONFIG:
            if (len >= 5) {
                uint32_t config = ((uint32_t)g_spi_rx_buffer[1] << 0) |
                                  ((uint32_t)g_spi_rx_buffer[2] << 8) |
                                  ((uint32_t)g_spi_rx_buffer[3] << 16) |
                                  ((uint32_t)g_spi_rx_buffer[4] << 24);
                memory_set_config(config);
                g_spi_tx_buffer[0] = STATUS_OK;
                spi_set_tx_length(1);
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_GET_CONFIG:
            {
                uint32_t config = memory_get_config();
                g_spi_tx_buffer[0] = STATUS_OK;
                g_spi_tx_buffer[1] = (config >> 0) & 0xFF;
                g_spi_tx_buffer[2] = (config >> 8) & 0xFF;
                g_spi_tx_buffer[3] = (config >> 16) & 0xFF;
                g_spi_tx_buffer[4] = (config >> 24) & 0xFF;
                spi_set_tx_length(5);
            }
            break;
            
        case CMD_LOAD_TOKEN:
            if (len >= 33) {
                memory_load_token(&g_spi_rx_buffer[1], len - 1);
                g_spi_tx_buffer[0] = STATUS_OK;
                spi_set_tx_length(1);
                g_app_state.token_loaded = true;
                DEBUG_PRINT("SPI: TOKEN_LOADED\r\n");
            } else {
                g_spi_tx_buffer[0] = STATUS_ERR;
                spi_set_tx_length(1);
            }
            break;
            
        case CMD_SAVE_TOKEN:
            {
                uint16_t token_len = 0;
                memory_save_token(&g_spi_tx_buffer[1], &token_len);
                g_spi_tx_buffer[0] = STATUS_OK;
                g_spi_tx_buffer[1] = token_len;
                spi_set_tx_length(2 + token_len);
            }
            break;
            
        case CMD_START_EMULATE:
            g_app_state.mode = MODE_EMULATION;
            rf_set_state(RF_STATE_LISTENING);
            g_spi_tx_buffer[0] = STATUS_OK;
            spi_set_tx_length(1);
            DEBUG_PRINT("SPI: EMULATION STARTED\r\n");
            break;
            
        case CMD_STOP_EMULATE:
            g_app_state.mode = MODE_IDLE;
            rf_set_state(RF_STATE_IDLE);
            g_spi_tx_buffer[0] = STATUS_OK;
            spi_set_tx_length(1);
            DEBUG_PRINT("SPI: EMULATION STOPPED\r\n");
            break;
            
        case CMD_GET_STATUS:
            g_spi_tx_buffer[0] = STATUS_OK;
            g_spi_tx_buffer[1] = g_app_state.mode;
            g_spi_tx_buffer[2] = rf_get_state();
            g_spi_tx_buffer[3] = g_app_state.token_loaded ? 1 : 0;
            g_spi_tx_buffer[4] = (memory_get_uid() >> 0) & 0xFF;
            g_spi_tx_buffer[5] = (memory_get_uid() >> 8) & 0xFF;
            g_spi_tx_buffer[6] = (memory_get_uid() >> 16) & 0xFF;
            g_spi_tx_buffer[7] = (memory_get_uid() >> 24) & 0xFF;
            spi_set_tx_length(8);
            break;
            
        case CMD_DEBUG_MODE:
            g_app_state.debug_enabled = true;
            g_spi_tx_buffer[0] = STATUS_OK;
            spi_set_tx_length(1);
            DEBUG_PRINT("SPI: DEBUG MODE ENABLED\r\n");
            break;
            
        default:
            g_spi_tx_buffer[0] = STATUS_ERR;
            spi_set_tx_length(1);
            DEBUG_PRINT("SPI: Unknown command 0x%02X\r\n", cmd);
            break;
    }
    
    // Reset RX buffer
    g_spi_rx_index = 0;
}

/*
 * Set TX buffer length for response
 */
static void spi_set_tx_length(uint8_t len) {
    g_spi_tx_index = 0;
    // Preload TX buffer with response data
    // The actual transmission happens when master clocks in
}

/*
 * SPI1 RX interrupt handler
 */
void __attribute__((interrupt(IPL2AUTO), vector(_SPI_1_VECTOR)))
spi1_rx_handler(void) {
    // Check if RX buffer has data
    while (SPI1STATbits.SPIRBF) {
        uint8_t data = SPI1BUF;
        
        if (g_spi_rx_index < SPI_RX_BUFFER_SIZE) {
            g_spi_rx_buffer[g_spi_rx_index++] = data;
            
            // Echo data back to master (if TX buffer ready)
            // In SPI, TX and RX happen simultaneously
            if (g_spi_tx_index < SPI_TX_BUFFER_SIZE) {
                SPI1BUF = g_spi_tx_buffer[g_spi_tx_index++];
            } else {
                SPI1BUF = 0xFF;
            }
        } else {
            // Buffer overflow - discard data
            SPI1BUF = 0xFF;  // Send error response
        }
    }
    
    // Clear interrupt flag
    IFS0bits.SPI1IF = 0;
}

/*
 * SPI1 error handler
 */
void __attribute__((interrupt(IPL3AUTO), vector(_SPI_1_ERR_VECTOR)))
spi1_err_handler(void) {
    // Check for overflow
    if (SPI1STATbits.SOV) {
        // Clear overflow flag by reading buffer
        (void)SPI1BUF;
    }
    
    // Clear error flag
    IFS0bits.SPI1EIF = 0;
}

/*
 * Check if SS is active (slave selected)
 */
bool spi_slave_selected(void) {
    return (PORTBbits.RB8 == 0);  // Active low
}

/*
 * Read data from SPI buffer
 */
uint8_t spi_read_byte(uint8_t index) {
    if (index < SPI_RX_BUFFER_SIZE) {
        return g_spi_rx_buffer[index];
    }
    return 0;
}

/*
 * Write data to SPI TX buffer
 */
void spi_write_byte(uint8_t index, uint8_t data) {
    if (index < SPI_TX_BUFFER_SIZE) {
        g_spi_tx_buffer[index] = data;
    }
}
