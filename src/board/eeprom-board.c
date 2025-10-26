#include <string.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "eeprom-board.h"

#define FLASH_PEKEY1 0x89ABCDEFU
#define FLASH_PEKEY2 0x02030405U
#define FLASH_PRGKEY1 0x8C9DAEBFU
#define FLASH_PRGKEY2 0x13141516U

static void FlashUnlock(void)
{
    if ((FLASH->PECR & FLASH_PECR_PELOCK) != 0U)
    {
        FLASH->PEKEYR = FLASH_PEKEY1;
        FLASH->PEKEYR = FLASH_PEKEY2;
    }
    if ((FLASH->PECR & FLASH_PECR_PRGLOCK) != 0U)
    {
        FLASH->PRGKEYR = FLASH_PRGKEY1;
        FLASH->PRGKEYR = FLASH_PRGKEY2;
    }
}

static void FlashLock(void)
{
    FLASH->PECR |= FLASH_PECR_PELOCK;
}

static void FlashWaitReady(void)
{
    while ((FLASH->SR & FLASH_SR_BSY) != 0U)
    {
    }
}

LmnStatus_t EepromMcuWriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t size)
{
    if (buffer == NULL || size == 0U)
    {
        return LMN_STATUS_ERROR;
    }

    if ((DATA_EEPROM_BASE + addr) >= DATA_EEPROM_BANK2_END)
    {
        return LMN_STATUS_ERROR;
    }

    FlashUnlock();
    CRITICAL_SECTION_BEGIN();

    for (uint16_t i = 0; i < size; i++)
    {
        FlashWaitReady();
        *(__IO uint8_t *)(DATA_EEPROM_BASE + addr + i) = buffer[i];
        FlashWaitReady();
        if ((FLASH->SR & (FLASH_SR_WRPERR | FLASH_SR_EOP | FLASH_SR_SIZERR)) != 0U)
        {
            FLASH->SR = FLASH_SR_WRPERR | FLASH_SR_EOP | FLASH_SR_SIZERR;
            CRITICAL_SECTION_END();
            FlashLock();
            return LMN_STATUS_ERROR;
        }
    }

    CRITICAL_SECTION_END();
    FlashLock();
    return LMN_STATUS_OK;
}

LmnStatus_t EepromMcuReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t size)
{
    if (buffer == NULL || size == 0U)
    {
        return LMN_STATUS_ERROR;
    }

    if ((DATA_EEPROM_BASE + addr) >= DATA_EEPROM_BANK2_END)
    {
        return LMN_STATUS_ERROR;
    }

    memcpy(buffer, (const void *)(DATA_EEPROM_BASE + addr), size);
    return LMN_STATUS_OK;
}

void EepromMcuSetDeviceAddr(uint8_t addr)
{
    (void)addr;
}

LmnStatus_t EepromMcuGetDeviceAddr(void)
{
    return 0;
}
