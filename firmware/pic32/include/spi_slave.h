/*
 * Hi-Tag 2 Emulator - SPI Slave Header
 */

#ifndef SPI_SLAVE_H
#define SPI_SLAVE_H

#include <stdint.h>
#include <stdbool.h>

// Initialize SPI slave
void spi_slave_init(void);

// Process SPI commands
void spi_slave_process(void);

// Check if slave is selected
bool spi_slave_selected(void);

// Buffer access
uint8_t spi_read_byte(uint8_t index);
void spi_write_byte(uint8_t index, uint8_t data);

#endif // SPI_SLAVE_H
