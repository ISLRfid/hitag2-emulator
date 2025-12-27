/*
 * Hi-Tag 2 Emulator - Serial Communication Header
 */

#ifndef HITAG2_SERIAL_H
#define HITAG2_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

/* Maximum buffer size */
#define UART_BUFFER_SIZE 64

/* Serial connection */
typedef struct SerialConnection SerialConnection;

/* Allocate/free connection */
SerialConnection* serial_alloc(void);
void serial_free(SerialConnection* serial);

/* Send command and receive response */
bool serial_send_command(
    SerialConnection* serial,
    uint8_t cmd,
    const uint8_t* data,
    uint8_t data_len,
    uint8_t* response,
    uint8_t* response_len
);

/* Raw byte operations */
bool serial_send_raw(SerialConnection* serial, const uint8_t* data, uint8_t len);
uint8_t serial_receive_raw(SerialConnection* serial, uint8_t* data, uint8_t max_len, uint32_t timeout);
bool serial_available(SerialConnection* serial);
uint8_t serial_read(SerialConnection* serial);
void serial_write(SerialConnection* serial, uint8_t byte);
void serial_flush(SerialConnection* serial);

#endif // HITAG2_SERIAL_H
