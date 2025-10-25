/*!
 * \file      storage.c
 *
 * \brief     Non-volatile storage implementation
 *
 * \details   Implements EEPROM emulation in Flash for STM32L072CZ.
 *            Uses HAL flash functions for write/erase operations.
 */
#include "storage.h"
#include "config.h"
#include "stm32l0xx_hal.h"
#include <string.h>

/* ============================================================================
 * PRIVATE DEFINITIONS
 * ========================================================================== */
#define STORAGE_MAGIC               0xDEADBEEF
#define STORAGE_VERSION             1

/* ============================================================================
 * PRIVATE TYPES
 * ========================================================================== */
typedef struct
{
    uint32_t Magic;
    uint32_t Version;
    StorageData_t Data;
} StorageBlock_t;

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static StorageData_t g_StorageCache;
static bool g_StorageInitialized = false;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static uint32_t Storage_CalculateCrc(const StorageData_t *data);
static bool Storage_FlashErase(uint32_t address, uint32_t size);
static bool Storage_FlashWrite(uint32_t address, const uint8_t *data, uint32_t size);
static bool Storage_FlashRead(uint32_t address, uint8_t *data, uint32_t size);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool Storage_Init(void)
{
    if (g_StorageInitialized)
    {
        return true;
    }

    /* Initialize flash interface */
    HAL_FLASH_Unlock();

    /* Load existing data or initialize with defaults */
    if (!Storage_Load(&g_StorageCache))
    {
        /* Initialize with default values */
        memset(&g_StorageCache, 0, sizeof(StorageData_t));

        /* Set default LoRaWAN parameters */
        g_StorageCache.TxDutyCycle = LORAWAN_DEFAULT_TDC;
        g_StorageCache.AdrEnabled = (LORAWAN_DEFAULT_ADR_STATE == LORAMAC_HANDLER_ADR_ON) ? 1 : 0;
        g_StorageCache.DataRate = LORAWAN_DEFAULT_DATARATE;
        g_StorageCache.TxPower = LORAWAN_DEFAULT_TX_POWER;
        g_StorageCache.Rx2DataRate = LORAWAN_RX2_DATARATE;
        g_StorageCache.Rx2Frequency = LORAWAN_RX2_FREQUENCY;
        g_StorageCache.Rx1Delay = LORAWAN_RX1_DELAY;
        g_StorageCache.Rx2Delay = LORAWAN_RX2_DELAY;
        g_StorageCache.FreqBand = LORAWAN_AU915_SUB_BAND;
        g_StorageCache.DeviceClass = LORAWAN_DEFAULT_CLASS;
        g_StorageCache.ConfirmedMsg = (LORAWAN_DEFAULT_CONFIRMED_MSG == LORAMAC_HANDLER_CONFIRMED_MSG) ? 1 : 0;
        g_StorageCache.AppPort = LORAWAN_DEFAULT_APP_PORT;

        /* Calculate and store CRC */
        g_StorageCache.Crc = Storage_CalculateCrc(&g_StorageCache);
    }

    g_StorageInitialized = true;
    HAL_FLASH_Lock();

    return true;
}

bool Storage_Load(StorageData_t *data)
{
    if (data == NULL)
    {
        return false;
    }

    StorageBlock_t block;

    /* Read from flash */
    if (!Storage_FlashRead(EEPROM_BASE_ADDRESS, (uint8_t*)&block, sizeof(StorageBlock_t)))
    {
        return false;
    }

    /* Verify magic number and version */
    if (block.Magic != STORAGE_MAGIC || block.Version != STORAGE_VERSION)
    {
        return false;
    }

    /* Verify CRC */
    uint32_t calculatedCrc = Storage_CalculateCrc(&block.Data);
    if (calculatedCrc != block.Data.Crc)
    {
        return false;
    }

    /* Copy data to output */
    memcpy(data, &block.Data, sizeof(StorageData_t));

    return true;
}

bool Storage_Save(const StorageData_t *data)
{
    if (data == NULL || !g_StorageInitialized)
    {
        return false;
    }

    StorageBlock_t block;

    /* Prepare block */
    block.Magic = STORAGE_MAGIC;
    block.Version = STORAGE_VERSION;
    memcpy(&block.Data, data, sizeof(StorageData_t));

    /* Calculate CRC */
    block.Data.Crc = Storage_CalculateCrc(&block.Data);

    HAL_FLASH_Unlock();

    /* Erase flash page */
    if (!Storage_FlashErase(EEPROM_BASE_ADDRESS, sizeof(StorageBlock_t)))
    {
        HAL_FLASH_Lock();
        return false;
    }

    /* Write to flash */
    bool result = Storage_FlashWrite(EEPROM_BASE_ADDRESS, (const uint8_t*)&block, sizeof(StorageBlock_t));

    HAL_FLASH_Lock();

    if (result)
    {
        /* Update cache */
        memcpy(&g_StorageCache, data, sizeof(StorageData_t));
    }

    return result;
}

bool Storage_Read(StorageKey_t key, uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || !g_StorageInitialized)
    {
        return false;
    }

    /* Read from cache */
    switch (key)
    {
        case STORAGE_KEY_DEVEUI:
            if (size >= 8) memcpy(buffer, g_StorageCache.DevEui, 8);
            else return false;
            break;

        case STORAGE_KEY_APPEUI:
            if (size >= 8) memcpy(buffer, g_StorageCache.AppEui, 8);
            else return false;
            break;

        case STORAGE_KEY_APPKEY:
            if (size >= 16) memcpy(buffer, g_StorageCache.AppKey, 16);
            else return false;
            break;

        case STORAGE_KEY_TDC:
            if (size >= 4) memcpy(buffer, &g_StorageCache.TxDutyCycle, 4);
            else return false;
            break;

        case STORAGE_KEY_ADR:
            if (size >= 1) *buffer = g_StorageCache.AdrEnabled;
            else return false;
            break;

        case STORAGE_KEY_DR:
            if (size >= 1) *buffer = g_StorageCache.DataRate;
            else return false;
            break;

        case STORAGE_KEY_TXP:
            if (size >= 1) *buffer = g_StorageCache.TxPower;
            else return false;
            break;

        case STORAGE_KEY_RX2DR:
            if (size >= 1) *buffer = g_StorageCache.Rx2DataRate;
            else return false;
            break;

        case STORAGE_KEY_RX2FQ:
            if (size >= 4) memcpy(buffer, &g_StorageCache.Rx2Frequency, 4);
            else return false;
            break;

        case STORAGE_KEY_CALIBRATION:
            if (size >= 32) memcpy(buffer, g_StorageCache.CalibrationData, 32);
            else return false;
            break;

        default:
            return false;
    }

    return true;
}

bool Storage_Write(StorageKey_t key, const uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || !g_StorageInitialized)
    {
        return false;
    }

    /* Update cache */
    switch (key)
    {
        case STORAGE_KEY_DEVEUI:
            if (size == 8) memcpy(g_StorageCache.DevEui, buffer, 8);
            else return false;
            break;

        case STORAGE_KEY_APPEUI:
            if (size == 8) memcpy(g_StorageCache.AppEui, buffer, 8);
            else return false;
            break;

        case STORAGE_KEY_APPKEY:
            if (size == 16) memcpy(g_StorageCache.AppKey, buffer, 16);
            else return false;
            break;

        case STORAGE_KEY_TDC:
            if (size == 4) memcpy(&g_StorageCache.TxDutyCycle, buffer, 4);
            else return false;
            break;

        case STORAGE_KEY_ADR:
            if (size == 1) g_StorageCache.AdrEnabled = *buffer;
            else return false;
            break;

        case STORAGE_KEY_DR:
            if (size == 1) g_StorageCache.DataRate = *buffer;
            else return false;
            break;

        case STORAGE_KEY_CALIBRATION:
            if (size <= 32) memcpy(g_StorageCache.CalibrationData, buffer, size);
            else return false;
            break;

        default:
            return false;
    }

    /* Save to flash */
    return Storage_Save(&g_StorageCache);
}

bool Storage_FactoryReset(void)
{
    if (!g_StorageInitialized)
    {
        return false;
    }

    HAL_FLASH_Unlock();
    bool result = Storage_FlashErase(EEPROM_BASE_ADDRESS, EEPROM_SIZE);
    HAL_FLASH_Lock();

    /* Reinitialize with defaults */
    g_StorageInitialized = false;
    return Storage_Init();
}

bool Storage_IsValid(void)
{
    StorageBlock_t block;

    if (!Storage_FlashRead(EEPROM_BASE_ADDRESS, (uint8_t*)&block, sizeof(StorageBlock_t)))
    {
        return false;
    }

    if (block.Magic != STORAGE_MAGIC || block.Version != STORAGE_VERSION)
    {
        return false;
    }

    uint32_t calculatedCrc = Storage_CalculateCrc(&block.Data);
    return (calculatedCrc == block.Data.Crc);
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static uint32_t Storage_CalculateCrc(const StorageData_t *data)
{
    /* Simple CRC32 calculation (use HAL CRC peripheral if available) */
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *ptr = (const uint8_t*)data;
    uint32_t size = sizeof(StorageData_t) - sizeof(uint32_t); /* Exclude CRC field */

    for (uint32_t i = 0; i < size; i++)
    {
        crc ^= ptr[i];
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }

    return ~crc;
}

static bool Storage_FlashErase(uint32_t address, uint32_t size)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;

    /* Calculate page to erase */
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = address;
    eraseInit.NbPages = (size + EEPROM_PAGE_SIZE - 1) / EEPROM_PAGE_SIZE;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseInit, &pageError);

    return (status == HAL_OK);
}

static bool Storage_FlashWrite(uint32_t address, const uint8_t *data, uint32_t size)
{
    HAL_StatusTypeDef status;

    /* Write data word by word (32-bit) */
    for (uint32_t i = 0; i < size; i += 4)
    {
        uint32_t word = 0;
        memcpy(&word, &data[i], (size - i >= 4) ? 4 : (size - i));

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, word);
        if (status != HAL_OK)
        {
            return false;
        }
    }

    return true;
}

static bool Storage_FlashRead(uint32_t address, uint8_t *data, uint32_t size)
{
    /* Direct memory read from flash */
    memcpy(data, (void*)address, size);
    return true;
}
