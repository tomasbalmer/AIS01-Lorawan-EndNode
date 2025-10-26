#ifndef SPI_BOARD_H
#define SPI_BOARD_H

#include "system/spi.h"

void SpiInit(Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss);
void SpiDeInit(Spi_t *obj);
void SpiFormat(Spi_t *obj, int8_t bits, int8_t cpol, int8_t cpha, int8_t slave);
void SpiFrequency(Spi_t *obj, uint32_t hz);
uint16_t SpiInOut(Spi_t *obj, uint16_t outData);

#endif /* SPI_BOARD_H */
