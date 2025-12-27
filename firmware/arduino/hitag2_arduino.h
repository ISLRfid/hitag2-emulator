/*
 * Hi-Tag 2 Emulator - Arduino Bridge Controller Header
 */

#ifndef HITAG2_ARDUINO_H
#define HITAG2_ARDUINO_H

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

// Pin definitions for Arduino Nano 33 BLE Rev2
#define PIN_SPI_MOSI    11
#define PIN_SPI_MISO    12
#define PIN_SPI_SCK     13
#define PIN_SPI_SS      10

#define PIN_RESET       2
#define PIN_TX_EN       3
#define PIN_IRQ         4

// Communication constants
#define UART_BAUD       115200
#define SPI_BAUD        4000000  // 4 MHz

#define UART_BUFFER_SIZE    64
#define SPI_BUFFER_SIZE     64

// UART protocol
#define SOF             0x02
#define EOF             0x03

// UART commands (from Flipper Zero)
#define CMD_PING            0x01
#define CMD_RESET           0x02
#define CMD_LOAD_TOKEN      0x10
#define CMD_SAVE_TOKEN      0x11
#define CMD_LIST_TOKENS     0x12
#define CMD_SELECT_TOKEN    0x13
#define CMD_SET_UID         0x20
#define CMD_SET_KEY         0x21
#define CMD_SET_CONFIG      0x22
#define CMD_GET_STATUS      0x30
#define CMD_START_EMULATE   0x40
#define CMD_STOP_EMULATE    0x41
#define CMD_READ_PAGE       0x50
#define CMD_WRITE_PAGE      0x51
#define CMD_STATUS          0x60
#define CMD_ERROR           0xFF

// SPI commands (to PIC32)
#define PIC_CMD_PING        0x01
#define PIC_CMD_RESET       0x02
#define PIC_CMD_READ_PAGE   0x10
#define PIC_CMD_WRITE_PAGE  0x20
#define PIC_CMD_SET_KEY     0x30
#define PIC_CMD_GET_KEY     0x31
#define PIC_CMD_SET_UID     0x40
#define PIC_CMD_GET_UID     0x41
#define PIC_CMD_SET_CONFIG  0x50
#define PIC_CMD_GET_CONFIG  0x51
#define PIC_CMD_LOAD_TOKEN  0x60
#define PIC_CMD_SAVE_TOKEN  0x61
#define PIC_CMD_START_EMULATE 0x70
#define PIC_CMD_STOP_EMULATE  0x71
#define PIC_CMD_GET_STATUS    0x80
#define PIC_CMD_DEBUG_MODE    0xA0

// Status codes
#define STATUS_OK            0x00
#define STATUS_ERR           0x01
#define STATUS_BUSY          0x02
#define STATUS_RDY           0x03

// Error codes
#define ERR_OK               0x00
#define ERR_INVALID_FRAME    0x01
#define ERR_UNKNOWN_COMMAND  0x02
#define ERR_INVALID_LENGTH   0x03
#define ERR_INVALID_TOKEN    0x04
#define ERR_NO_TOKEN         0x05
#define ERR_NO_SPACE         0x06
#define ERR_PIC32            0x07

// Token structure
typedef struct {
    uint32_t uid;           // Page 0: Serial number
    uint32_t config;        // Page 1: Configuration
    uint8_t key[6];         // Pages 2-3: Secret key
    uint32_t user_data[4];  // Pages 4-7: User data
} Token;

// Token management
Token create_default_token();

// Command callbacks
typedef void (*CommandCallback)(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);

// Command implementations
void cmd_ping(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_reset(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_load_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_save_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_list_tokens(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_select_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_set_uid(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_set_key(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_set_config(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_get_status(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_start_emulate(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_stop_emulate(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_read_page(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void cmd_write_page(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);

// Communication functions
void send_response(uint8_t cmd, const uint8_t* data, uint8_t len);
void send_error(uint8_t error_code);
void send_raw(const uint8_t* data, uint8_t len);
bool send_to_pic32(uint8_t cmd, const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);
void reset_pic32();

// Event handlers
void process_uart_command();
void handle_pic32_interrupt();
void update_status();

#endif // HITAG2_ARDUINO_H
