#include "stm32l0xx.h"
#include "utilities.h"
#include "gpio.h"
#include "gpio-board.h"
#include "spi-board.h"

#ifndef SPI_DATASIZE_8BIT
#define SPI_DATASIZE_8BIT 0
#define SPI_DATASIZE_16BIT 1
#endif

static uint32_t SpiComputePrescaler(uint32_t targetHz)
{
    if (targetHz == 0U)
    {
        return 7U;
    }

    uint32_t spiClk = SystemCoreClock;
    uint32_t prescaler = 0U;

    while ((prescaler < 7U) && (spiClk > targetHz))
    {
        prescaler++;
        spiClk >>= 1;
    }

    return prescaler;
}

void SpiInit(Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss)
{
    if (obj == NULL)
    {
        return;
    }

    obj->SpiId = spiId;

    if (spiId != SPI_1)
    {
        return;
    }

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GpioInit(&obj->Mosi, mosi, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF0_SPI1);
    GpioInit(&obj->Miso, miso, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF0_SPI1);
    GpioInit(&obj->Sclk, sclk, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF0_SPI1);

    if (nss != NC)
    {
        GpioInit(&obj->Nss, nss, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1);
    }

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR2 = (7U << 8); /* 8-bit data size */
    SpiFrequency(obj, 8000000U);
    SpiFormat(obj, SPI_DATASIZE_8BIT, 0, 0, 0);
    SPI1->CR1 |= SPI_CR1_SPE;
}

void SpiDeInit(Spi_t *obj)
{
    if ((obj == NULL) || (obj->SpiId != SPI_1))
    {
        return;
    }

    SPI1->CR1 &= ~SPI_CR1_SPE;
    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
}

void SpiFormat(Spi_t *obj, int8_t bits, int8_t cpol, int8_t cpha, int8_t slave)
{
    (void)slave;

    if ((obj == NULL) || (obj->SpiId != SPI_1))
    {
        return;
    }

    SPI1->CR1 &= ~SPI_CR1_SPE;

    if (bits == SPI_DATASIZE_16BIT)
    {
        SPI1->CR2 = (SPI1->CR2 & ~(0xFU << 8)) | (15U << 8);
    }
    else
    {
        SPI1->CR2 = (SPI1->CR2 & ~(0xFU << 8)) | (7U << 8);
    }

    if (cpol)
    {
        SPI1->CR1 |= SPI_CR1_CPOL;
    }
    else
    {
        SPI1->CR1 &= ~SPI_CR1_CPOL;
    }

    if (cpha)
    {
        SPI1->CR1 |= SPI_CR1_CPHA;
    }
    else
    {
        SPI1->CR1 &= ~SPI_CR1_CPHA;
    }

    SPI1->CR1 |= SPI_CR1_SPE;
}

void SpiFrequency(Spi_t *obj, uint32_t hz)
{
    if ((obj == NULL) || (obj->SpiId != SPI_1) || (hz == 0U))
    {
        return;
    }

    uint32_t prescaler = SpiComputePrescaler(hz) & 0x7U;
    SPI1->CR1 &= ~(7U << 3);
    SPI1->CR1 |= (prescaler << 3);
}

uint16_t SpiInOut(Spi_t *obj, uint16_t outData)
{
    if ((obj == NULL) || (obj->SpiId != SPI_1))
    {
        return 0xFFFF;
    }

    while ((SPI1->SR & SPI_SR_TXE) == 0U)
    {
    }
    *((__IO uint8_t *)&SPI1->DR) = (uint8_t)(outData & 0xFFU);

    while ((SPI1->SR & SPI_SR_RXNE) == 0U)
    {
    }
    return (uint16_t)(SPI1->DR & 0xFFU);
}
