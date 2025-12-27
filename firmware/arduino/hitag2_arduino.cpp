/*
 * Hi-Tag 2 Emulator - Arduino Bridge Controller
 * Bridges Flipper Zero (UART) and PIC32 (SPI)
 * 
 * Target: Arduino Nano 33 BLE Rev2 (nRF52840)
 * Purpose: Control interface for Hi-Tag 2 emulation
 */

#include "hitag2_arduino.h"

// Token storage (in RAM, can be extended to external flash)
#define MAX_TOKENS 10
#define TOKEN_SIZE 32

static Token tokens[MAX_TOKENS];
static int selected_token_index = -1;
static bool emulation_active = false;

// Command callbacks
typedef void (*CommandCallback)(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len);

struct CommandEntry {
    uint8_t cmd;
    const char* name;
    CommandCallback callback;
};

static const CommandEntry command_table[] = {
    {CMD_PING, "PING", cmd_ping},
    {CMD_RESET, "RESET", cmd_reset},
    {CMD_LOAD_TOKEN, "LOAD_TOKEN", cmd_load_token},
    {CMD_SAVE_TOKEN, "SAVE_TOKEN", cmd_save_token},
    {CMD_LIST_TOKENS, "LIST_TOKENS", cmd_list_tokens},
    {CMD_SELECT_TOKEN, "SELECT_TOKEN", cmd_select_token},
    {CMD_SET_UID, "SET_UID", cmd_set_uid},
    {CMD_SET_KEY, "SET_KEY", cmd_set_key},
    {CMD_SET_CONFIG, "SET_CONFIG", cmd_set_config},
    {CMD_GET_STATUS, "GET_STATUS", cmd_get_status},
    {CMD_START_EMULATE, "START_EMULATE", cmd_start_emulate},
    {CMD_STOP_EMULATE, "STOP_EMULATE", cmd_stop_emulate},
    {CMD_READ_PAGE, "READ_PAGE", cmd_read_page},
    {CMD_WRITE_PAGE, "WRITE_PAGE", cmd_write_page},
};

#define NUM_COMMANDS (sizeof(command_table) / sizeof(command_table[0]))

// Buffer for communication
static uint8_t uart_rx_buffer[UART_BUFFER_SIZE];
static uint8_t uart_tx_buffer[UART_BUFFER_SIZE];
static uint8_t spi_rx_buffer[SPI_BUFFER_SIZE];
static uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];
static volatile uint8_t uart_rx_index = 0;
static volatile uint8_t uart_tx_index = 0;
static bool uart_frame_complete = false;

// LED pin for status
#define STATUS_LED LED_BUILTIN

/*
 * Initialize system
 */
void setup() {
    // Initialize serial for Flipper Zero
    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }
    
    // Initialize SPI for PIC32
    SPI.begin();
    pinMode(PIN_SPI_SS, OUTPUT);
    digitalWrite(PIN_SPI_SS, HIGH);  // Deselect PIC32
    
    // Initialize control pins
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, HIGH);  // Release reset
    
    pinMode(PIN_TX_EN, OUTPUT);
    digitalWrite(PIN_TX_EN, LOW);  // TX disabled
    
    pinMode(PIN_IRQ, INPUT);
    
    // Initialize LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);
    
    // Initialize token storage
    for (int i = 0; i < MAX_TOKENS; i++) {
        memset(&tokens[i], 0, sizeof(Token));
    }
    
    // Add default token
    tokens[0] = create_default_token();
    selected_token_index = 0;
    
    // Status LED on
    digitalWrite(STATUS_LED, HIGH);
    
    Serial.println(F("Hi-Tag 2 Emulator Bridge initialized"));
    Serial.println(F("Ready for commands from Flipper Zero"));
    
    // Send initial ready signal
    send_response(CMD_PING, NULL, 0);
}

/*
 * Create default token
 */
Token create_default_token() {
    Token token;
    token.uid = 0xDEADBEEF;
    token.config = 0x00000000;
    token.key[0] = 0x12;
    token.key[1] = 0x34;
    token.key[2] = 0x56;
    token.key[3] = 0x78;
    token.key[4] = 0x9A;
    token.key[5] = 0xBC;
    token.user_data[0] = 0x00000000;
    token.user_data[1] = 0x00000000;
    token.user_data[2] = 0x00000000;
    token.user_data[3] = 0x00000000;
    return token;
}

/*
 * Main loop
 */
void loop() {
    // Process UART commands from Flipper Zero
    if (uart_frame_complete) {
        process_uart_command();
    }
    
    // Check for PIC32 interrupts
    if (digitalRead(PIN_IRQ) == LOW) {
        handle_pic32_interrupt();
    }
    
    // Update status
    update_status();
    
    // Small delay to prevent tight loop
    delay(10);
}

/*
 * UART event handlers
 */
void serialEvent() {
    while (Serial.available() > 0) {
        uint8_t byte = Serial.read();
        
        if (byte == SOF) {
            // Start of frame
            uart_rx_index = 0;
            uart_rx_buffer[uart_rx_index++] = byte;
        } else if (uart_rx_index > 0 && uart_rx_index < UART_BUFFER_SIZE) {
            uart_rx_buffer[uart_rx_index++] = byte;
            
            // Check for end of frame (newline)
            if (byte == '\n' || uart_rx_index >= UART_BUFFER_SIZE) {
                uart_frame_complete = true;
            }
        }
    }
}

/*
 * Process UART command from Flipper Zero
 */
void process_uart_command() {
    uint8_t response_len = 0;
    
    if (uart_rx_index < 2) {
        send_error(ERR_INVALID_FRAME);
        goto done;
    }
    
    uint8_t cmd = uart_rx_buffer[1];
    uint8_t len = uart_rx_index - 2;  // Exclude SOF and length
    
    // Find command in table
    const CommandEntry* entry = NULL;
    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (command_table[i].cmd == cmd) {
            entry = &command_table[i];
            break;
        }
    }
    
    if (entry == NULL) {
        Serial.print(F("Unknown command: 0x"));
        Serial.println(cmd, HEX);
        send_error(ERR_UNKNOWN_COMMAND);
        goto done;
    }
    
    // Execute command
    uint8_t response_data[UART_BUFFER_SIZE];
    entry->callback(&uart_rx_buffer[2], len, response_data, &response_len);
    
    // Send response
    send_response(cmd, response_data, response_len);
    
done:
    uart_frame_complete = false;
    uart_rx_index = 0;
}

/*
 * Send response to Flipper Zero
 */
void send_response(uint8_t cmd, const uint8_t* data, uint8_t len) {
    Serial.write(SOF);
    Serial.write(cmd);
    Serial.write(len);
    if (len > 0 && data != NULL) {
        Serial.write(data, len);
    }
    // Send checksum
    uint8_t checksum = SOF ^ cmd ^ len;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    Serial.write(checksum);
    Serial.write('\n');
}

/*
 * Send error response
 */
void send_error(uint8_t error_code) {
    send_response(CMD_ERROR, &error_code, 1);
}

/*
 * Send raw response (for debug)
 */
void send_raw(const uint8_t* data, uint8_t len) {
    Serial.write(data, len);
}

/*
 * Send token to PIC32 via SPI
 */
bool send_to_pic32(uint8_t cmd, const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    // Build SPI frame
    spi_tx_buffer[0] = cmd;
    spi_tx_buffer[1] = len;
    for (uint8_t i = 0; i < len && i < (SPI_BUFFER_SIZE - 2); i++) {
        spi_tx_buffer[2 + i] = data[i];
    }
    
    // Select PIC32
    digitalWrite(PIN_SPI_SS, LOW);
    
    // Send/receive via SPI
    uint8_t tx_len = 2 + len;
    uint8_t rx_len = 0;
    
    for (uint8_t i = 0; i < SPI_BUFFER_SIZE; i++) {
        uint8_t tx_byte = (i < tx_len) ? spi_tx_buffer[i] : 0xFF;
        uint8_t rx_byte = SPI.transfer(tx_byte);
        
        if (i < 2) {
            // Header (status and length)
            response[rx_len++] = rx_byte;
        } else if (i < (2 + response[1])) {
            // Data
            response[rx_len++] = rx_byte;
        }
        
        // Check if done receiving
        if (i >= 2 && i >= (2 + response[1])) {
            break;
        }
    }
    
    // Deselect PIC32
    digitalWrite(PIN_SPI_SS, HIGH);
    
    *response_len = rx_len;
    
    // Check status
    if (response[0] != STATUS_OK) {
        Serial.print(F("PIC32 error: 0x"));
        Serial.println(response[0], HEX);
        return false;
    }
    
    return true;
}

/*
 * Reset PIC32
 */
void reset_pic32() {
    digitalWrite(PIN_RESET, LOW);
    delay(10);
    digitalWrite(PIN_RESET, HIGH);
    delay(100);
}

/*
 * Handle PIC32 interrupt
 */
void handle_pic32_interrupt() {
    // Read IRQ status from PIC32
    uint8_t response[SPI_BUFFER_SIZE];
    uint8_t response_len = 0;
    
    if (send_to_pic32(CMD_GET_STATUS, NULL, 0, response, &response_len)) {
        // Forward status to Flipper Zero
        send_response(CMD_STATUS, response + 2, response_len - 2);
    }
}

/*
 * Update status LED
 */
void update_status() {
    // Blink when active
    if (emulation_active) {
        digitalWrite(STATUS_LED, (millis() / 500) % 2);
    }
}

/*
 * Command implementations
 */
void cmd_ping(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    response[0] = 0x00;  // Pong
    *response_len = 1;
}

void cmd_reset(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    reset_pic32();
    emulation_active = false;
    selected_token_index = -1;
    response[0] = 0x00;
    *response_len = 1;
}

void cmd_load_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < TOKEN_SIZE) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    // Parse token data
    Token token;
    memcpy(&token, data, TOKEN_SIZE);
    
    // Store in next available slot
    int slot = -1;
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (tokens[i].uid == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        response[0] = ERR_NO_SPACE;
        *response_len = 1;
        return;
    }
    
    tokens[slot] = token;
    response[0] = slot + 1;  // Return slot number (1-based)
    response[1] = token.uid & 0xFF;
    response[2] = (token.uid >> 8) & 0xFF;
    response[3] = (token.uid >> 16) & 0xFF;
    response[4] = (token.uid >> 24) & 0xFF;
    *response_len = 5;
    
    Serial.print(F("Token loaded into slot "));
    Serial.println(slot + 1);
}

void cmd_save_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (selected_token_index < 0) {
        response[0] = ERR_NO_TOKEN;
        *response_len = 1;
        return;
    }
    
    Token* token = &tokens[selected_token_index];
    memcpy(response, token, TOKEN_SIZE);
    *response_len = TOKEN_SIZE;
}

void cmd_list_tokens(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    uint8_t pos = 0;
    
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (tokens[i].uid != 0) {
            response[pos++] = i + 1;  // Slot number (1-based)
            response[pos++] = tokens[i].uid & 0xFF;
            response[pos++] = (tokens[i].uid >> 8) & 0xFF;
            response[pos++] = (tokens[i].uid >> 16) & 0xFF;
            response[pos++] = (tokens[i].uid >> 24) & 0xFF;
        }
    }
    
    *response_len = pos;
}

void cmd_select_token(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 1) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    int slot = data[0] - 1;  // Convert to 0-based
    
    if (slot < 0 || slot >= MAX_TOKENS || tokens[slot].uid == 0) {
        response[0] = ERR_INVALID_TOKEN;
        *response_len = 1;
        return;
    }
    
    selected_token_index = slot;
    
    // Send token to PIC32
    if (send_to_pic32(CMD_LOAD_TOKEN, (uint8_t*)&tokens[slot], TOKEN_SIZE, response, response_len)) {
        emulation_active = false;
        response[0] = 0x00;
        *response_len = 1;
        
        Serial.print(F("Token selected: UID=0x"));
        Serial.println(tokens[slot].uid, HEX);
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_set_uid(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 4) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    uint32_t uid = ((uint32_t)data[0]) |
                   ((uint32_t)data[1] << 8) |
                   ((uint32_t)data[2] << 16) |
                   ((uint32_t)data[3] << 24);
    
    // Update selected token
    if (selected_token_index >= 0) {
        tokens[selected_token_index].uid = uid;
    }
    
    // Send to PIC32
    uint8_t cmd_data[4];
    cmd_data[0] = data[0];
    cmd_data[1] = data[1];
    cmd_data[2] = data[2];
    cmd_data[3] = data[3];
    
    if (send_to_pic32(CMD_SET_UID, cmd_data, 4, response, response_len)) {
        response[0] = 0x00;
        *response_len = 1;
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_set_key(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 6) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    // Update selected token
    if (selected_token_index >= 0) {
        memcpy(tokens[selected_token_index].key, data, 6);
    }
    
    // Send to PIC32
    if (send_to_pic32(CMD_SET_KEY, data, 6, response, response_len)) {
        response[0] = 0x00;
        *response_len = 1;
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_set_config(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 4) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    // Update selected token
    if (selected_token_index >= 0) {
        memcpy(&tokens[selected_token_index].config, data, 4);
    }
    
    // Send to PIC32
    if (send_to_pic32(CMD_SET_CONFIG, data, 4, response, response_len)) {
        response[0] = 0x00;
        *response_len = 1;
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_get_status(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    response[0] = emulation_active ? 1 : 0;
    response[1] = selected_token_index + 1;  // 0 = none
    response[2] = 0x00;  // Reserved
    
    if (selected_token_index >= 0) {
        uint32_t uid = tokens[selected_token_index].uid;
        response[3] = uid & 0xFF;
        response[4] = (uid >> 8) & 0xFF;
        response[5] = (uid >> 16) & 0xFF;
        response[6] = (uid >> 24) & 0xFF;
        *response_len = 7;
    } else {
        *response_len = 3;
    }
}

void cmd_start_emulate(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (selected_token_index < 0) {
        response[0] = ERR_NO_TOKEN;
        *response_len = 1;
        return;
    }
    
    // Send start command to PIC32
    if (send_to_pic32(CMD_START_EMULATE, NULL, 0, response, response_len)) {
        emulation_active = true;
        response[0] = 0x00;
        *response_len = 1;
        Serial.println(F("Emulation started"));
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_stop_emulate(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    // Send stop command to PIC32
    if (send_to_pic32(CMD_STOP_EMULATE, NULL, 0, response, response_len)) {
        emulation_active = false;
        response[0] = 0x00;
        *response_len = 1;
        Serial.println(F("Emulation stopped"));
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_read_page(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 1) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    uint8_t page = data[0];
    
    if (send_to_pic32(CMD_READ_PAGE, data, 1, response, response_len)) {
        // response[0] = status, response[1] = length, response[2..5] = data
        // Already formatted correctly
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}

void cmd_write_page(const uint8_t* data, uint8_t len, uint8_t* response, uint8_t* response_len) {
    if (len < 5) {
        response[0] = ERR_INVALID_LENGTH;
        *response_len = 1;
        return;
    }
    
    if (send_to_pic32(CMD_WRITE_PAGE, data, len, response, response_len)) {
        response[0] = 0x00;
        *response_len = 1;
    } else {
        response[0] = ERR_PIC32;
        *response_len = 1;
    }
}
