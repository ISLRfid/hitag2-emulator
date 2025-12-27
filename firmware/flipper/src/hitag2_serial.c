/*
 * Hi-Tag 2 Emulator - Serial Communication Helper
 * UART communication with Arduino bridge
 */

#include "hitag2_serial.h"
#include <furi.h>
#include <furi_hal.h>

/* Serial connection structure */
struct SerialConnection {
    FuriHalUsbVirtualSerial* usb;
    FuriHalUartConfig uart_config;
    uint8_t rx_buffer[UART_BUFFER_SIZE];
    uint8_t rx_index;
    uint8_t tx_buffer[UART_BUFFER_SIZE];
    bool frame_ready;
};

/* Allocate serial connection */
SerialConnection* serial_alloc(void) {
    SerialConnection* serial = malloc(sizeof(SerialConnection));
    
    serial->rx_index = 0;
    serial->frame_ready = false;
    
    // Configure UART
    furi_hal_uart_init(FuriHalUartIdUSART1, 115200);
    
    return serial;
}

/* Free serial connection */
void serial_free(SerialConnection* serial) {
    furi_hal_uart_deinit(FuriHalUartIdUSART1);
    free(serial);
}

/* Send command and receive response */
bool serial_send_command(
    SerialConnection* serial,
    uint8_t cmd,
    const uint8_t* data,
    uint8_t data_len,
    uint8_t* response,
    uint8_t* response_len
) {
    uint8_t tx_buffer[UART_BUFFER_SIZE];
    uint8_t tx_index = 0;
    
    // Build frame
    tx_buffer[tx_index++] = SOF;
    tx_buffer[tx_index++] = cmd;
    tx_buffer[tx_index++] = data_len;
    
    for (uint8_t i = 0; i < data_len && tx_index < UART_BUFFER_SIZE - 2; i++) {
        tx_buffer[tx_index++] = data[i];
    }
    
    // Calculate checksum
    uint8_t checksum = SOF ^ cmd ^ data_len;
    for (uint8_t i = 0; i < data_len; i++) {
        checksum ^= data[i];
    }
    tx_buffer[tx_index++] = checksum;
    
    // Send via UART
    furi_hal_uart_tx(FuriHalUartIdUSART1, tx_buffer, tx_index);
    
    // Wait for response
    serial->rx_index = 0;
    uint32_t timeout = furi_get_tick() + 1000;  // 1 second timeout
    
    while (furi_get_tick() < timeout) {
        if (furi_hal_uart_is_rx_not_empty(FuriHalUartIdUSART1)) {
            uint8_t byte = furi_hal_uart_rx(FuriHalUartIdUSART1);
            
            if (serial->rx_index < UART_BUFFER_SIZE) {
                serial->rx_buffer[serial->rx_index++] = byte;
                
                // Check for complete frame (newline)
                if (byte == '\n' && serial->rx_index >= 4) {
                    serial->frame_ready = true;
                    break;
                }
            }
        }
        furi_delay_ms(1);
    }
    
    // Parse response
    if (serial->frame_ready && serial->rx_index >= 4) {
        uint8_t sof = serial->rx_buffer[0];
        uint8_t resp_cmd = serial->rx_buffer[1];
        uint8_t resp_len = serial->rx_buffer[2];
        uint8_t checksum = serial->rx_buffer[3 + resp_len];
        
        // Verify checksum
        uint8_t calc_checksum = sof ^ resp_cmd ^ resp_len;
        for (uint8_t i = 0; i < resp_len; i++) {
            calc_checksum ^= serial->rx_buffer[3 + i];
        }
        
        if (calc_checksum == checksum && resp_cmd == cmd) {
            memcpy(response, &serial->rx_buffer[3], resp_len);
            *response_len = resp_len;
            serial->frame_ready = false;
            serial->rx_index = 0;
            return true;
        }
    }
    
    serial->frame_ready = false;
    serial->rx_index = 0;
    return false;
}

/* Send raw bytes */
bool serial_send_raw(SerialConnection* serial, const uint8_t* data, uint8_t len) {
    furi_hal_uart_tx(FuriHalUartIdUSART1, data, len);
    return true;
}

/* Receive raw bytes */
uint8_t serial_receive_raw(SerialConnection* serial, uint8_t* data, uint8_t max_len, uint32_t timeout) {
    uint8_t received = 0;
    uint32_t start = furi_get_tick();
    
    while (received < max_len && (furi_get_tick() - start) < timeout) {
        if (furi_hal_uart_is_rx_not_empty(FuriHalUartIdUSART1)) {
            data[received++] = furi_hal_uart_rx(FuriHalUartIdUSART1);
        }
        furi_delay_ms(1);
    }
    
    return received;
}

/* Check if data available */
bool serial_available(SerialConnection* serial) {
    UNUSED(serial);
    return furi_hal_uart_is_rx_not_empty(FuriHalUartIdUSART1);
}

/* Read single byte */
uint8_t serial_read(SerialConnection* serial) {
    return furi_hal_uart_rx(FuriHalUartIdUSART1);
}

/* Write single byte */
void serial_write(SerialConnection* serial, uint8_t byte) {
    furi_hal_uart_tx(FuriHalUartIdUSART1, &byte, 1);
}

/* Flush RX buffer */
void serial_flush(SerialConnection* serial) {
    serial->rx_index = 0;
    serial->frame_ready = false;
    
    while (furi_hal_uart_is_rx_not_empty(FuriHalUartIdUSART1)) {
        furi_hal_uart_rx(FuriHalUartIdUSART1);
    }
}
