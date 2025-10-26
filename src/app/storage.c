/*!
 * \file      storage.c
 *
 * \brief     Non-volatile storage implementation
 *
 * \details   Implements EEPROM emulation in Flash for STM32L072CZ.
 *            Uses HAL flash functions for write/erase operations.
 */
#include <stdbool.h>
#include <string.h>
#include "storage.h"
#include "config.h"
#include "eeprom-board.h"

/* ============================================================================
 * PRIVATE DEFINITIONS
 * ========================================================================== */
#define STORAGE_MAGIC 0xDEADBEEF
#define STORAGE_VERSION 1

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
    /* Load existing data or initialize with defaults */
    if (!Storage_Load(&g_StorageCache))
    {
        /* Initialize with default values */
        memset(&g_StorageCache, 0, sizeof(StorageData_t));

        /* Set default LoRaWAN parameters */
        g_StorageCache.TxDutyCycle = LORAWAN_DEFAULT_TDC;
        g_StorageCache.AdrEnabled = (LORAWAN_DEFAULT_ADR_STATE != 0);
        g_StorageCache.DataRate = LORAWAN_DEFAULT_DATARATE;
        g_StorageCache.TxPower = LORAWAN_DEFAULT_TX_POWER;
        g_StorageCache.Rx2DataRate = LORAWAN_RX2_DATARATE;
        g_StorageCache.Rx2Frequency = LORAWAN_RX2_FREQUENCY;
        g_StorageCache.Rx1Delay = LORAWAN_RX1_DELAY;
        g_StorageCache.Rx2Delay = LORAWAN_RX2_DELAY;
        g_StorageCache.FreqBand = LORAWAN_AU915_SUB_BAND;
        g_StorageCache.DeviceClass = LORAWAN_DEFAULT_CLASS;
        g_StorageCache.ConfirmedMsg = (LORAWAN_DEFAULT_CONFIRMED_MSG != 0);
        g_StorageCache.AppPort = LORAWAN_DEFAULT_APP_PORT;
        g_StorageCache.FrameCounterUp = 0;
        g_StorageCache.FrameCounterDown = 0;

        /* Calculate and store CRC */
        g_StorageCache.Crc = Storage_CalculateCrc(&g_StorageCache);
    }

    g_StorageInitialized = true;
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
    if (!Storage_FlashRead(EEPROM_BASE_ADDRESS, (uint8_t *)&block, sizeof(StorageBlock_t)))
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

    /* Erase flash page */
    if (!Storage_FlashErase(EEPROM_BASE_ADDRESS, sizeof(StorageBlock_t)))
    {
        return false;
    }

    /* Write to flash */
    bool result = Storage_FlashWrite(EEPROM_BASE_ADDRESS, (const uint8_t *)&block, sizeof(StorageBlock_t));

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
        if (size >= 8)
            memcpy(buffer, g_StorageCache.DevEui, 8);
        else
            return false;
        break;

    case STORAGE_KEY_APPEUI:
        if (size >= 8)
            memcpy(buffer, g_StorageCache.AppEui, 8);
        else
            return false;
        break;

    case STORAGE_KEY_APPKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.AppKey, 16);
        else
            return false;
        break;

    case STORAGE_KEY_DEVADDR:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.DevAddr, 4);
        else
            return false;
        break;

    case STORAGE_KEY_NWKSKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.NwkSKey, 16);
        else
            return false;
        break;

    case STORAGE_KEY_APPSKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.AppSKey, 16);
        else
            return false;
        break;

    case STORAGE_KEY_TDC:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.TxDutyCycle, 4);
        else
            return false;
        break;

    case STORAGE_KEY_ADR:
        if (size >= 1)
            *buffer = g_StorageCache.AdrEnabled;
        else
            return false;
        break;

    case STORAGE_KEY_DR:
        if (size >= 1)
            *buffer = g_StorageCache.DataRate;
        else
            return false;
        break;

    case STORAGE_KEY_TXP:
        if (size >= 1)
            *buffer = g_StorageCache.TxPower;
        else
            return false;
        break;

    case STORAGE_KEY_RX2DR:
        if (size >= 1)
            *buffer = g_StorageCache.Rx2DataRate;
        else
            return false;
        break;

    case STORAGE_KEY_RX2FQ:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx2Frequency, 4);
        else
            return false;
        break;

    case STORAGE_KEY_RX1DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx1Delay, 4);
        else
            return false;
        break;

    case STORAGE_KEY_RX2DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx2Delay, 4);
        else
            return false;
        break;

    case STORAGE_KEY_FREQBAND:
        if (size >= 1)
            *buffer = g_StorageCache.FreqBand;
        else
            return false;
        break;

    case STORAGE_KEY_CLASS:
        if (size >= 1)
            *buffer = g_StorageCache.DeviceClass;
        else
            return false;
        break;

    case STORAGE_KEY_CONFIRMED:
        if (size >= 1)
            *buffer = g_StorageCache.ConfirmedMsg;
        else
            return false;
        break;

    case STORAGE_KEY_PORT:
        if (size >= 1)
            *buffer = g_StorageCache.AppPort;
        else
            return false;
        break;

    case STORAGE_KEY_FCNTUP:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.FrameCounterUp, 4);
        else
            return false;
        break;

    case STORAGE_KEY_FCNTDOWN:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.FrameCounterDown, 4);
        else
            return false;
        break;

    case STORAGE_KEY_CALIBRATION:
        if (size >= sizeof(g_StorageCache.CalibrationData))
        {
            memcpy(buffer, g_StorageCache.CalibrationData, sizeof(g_StorageCache.CalibrationData));
        }
        else
            return false;
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

    switch (key)
    {
    case STORAGE_KEY_DEVEUI:
        if (size != 8)
            return false;
        memcpy(g_StorageCache.DevEui, buffer, 8);
        break;

    case STORAGE_KEY_APPEUI:
        if (size != 8)
            return false;
        memcpy(g_StorageCache.AppEui, buffer, 8);
        break;

    case STORAGE_KEY_APPKEY:
        if (size != 16)
            return false;
        memcpy(g_StorageCache.AppKey, buffer, 16);
        break;

    case STORAGE_KEY_DEVADDR:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.DevAddr, buffer, 4);
        break;

    case STORAGE_KEY_NWKSKEY:
        if (size != 16)
            return false;
        memcpy(g_StorageCache.NwkSKey, buffer, 16);
        break;

    case STORAGE_KEY_APPSKEY:
        if (size != 16)
            return false;
        memcpy(g_StorageCache.AppSKey, buffer, 16);
        break;

    case STORAGE_KEY_TDC:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.TxDutyCycle, buffer, 4);
        break;

    case STORAGE_KEY_ADR:
        if (size != 1)
            return false;
        g_StorageCache.AdrEnabled = *buffer;
        break;

    case STORAGE_KEY_DR:
        if (size != 1)
            return false;
        g_StorageCache.DataRate = *buffer;
        break;

    case STORAGE_KEY_TXP:
        if (size != 1)
            return false;
        g_StorageCache.TxPower = *buffer;
        break;

    case STORAGE_KEY_RX2DR:
        if (size != 1)
            return false;
        g_StorageCache.Rx2DataRate = *buffer;
        break;

    case STORAGE_KEY_RX2FQ:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.Rx2Frequency, buffer, 4);
        break;

    case STORAGE_KEY_RX1DL:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.Rx1Delay, buffer, 4);
        break;

    case STORAGE_KEY_RX2DL:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.Rx2Delay, buffer, 4);
        break;

    case STORAGE_KEY_FREQBAND:
        if (size != 1)
            return false;
        g_StorageCache.FreqBand = *buffer;
        break;

    case STORAGE_KEY_CLASS:
        if (size != 1)
            return false;
        g_StorageCache.DeviceClass = *buffer;
        break;

    case STORAGE_KEY_CONFIRMED:
        if (size != 1)
            return false;
        g_StorageCache.ConfirmedMsg = *buffer;
        break;

    case STORAGE_KEY_PORT:
        if (size != 1)
            return false;
        g_StorageCache.AppPort = *buffer;
        break;

    case STORAGE_KEY_FCNTUP:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.FrameCounterUp, buffer, 4);
        break;

    case STORAGE_KEY_FCNTDOWN:
        if (size != 4)
            return false;
        memcpy(&g_StorageCache.FrameCounterDown, buffer, 4);
        break;

    case STORAGE_KEY_CALIBRATION:
        if (size > sizeof(g_StorageCache.CalibrationData))
            return false;
        memcpy(g_StorageCache.CalibrationData, buffer, size);
        break;

    default:
        return false;
    }

    return Storage_Save(&g_StorageCache);
}

bool Storage_FactoryReset(void)
{
    if (!g_StorageInitialized)
    {
        return false;
    }

    bool result = Storage_FlashErase(EEPROM_BASE_ADDRESS, EEPROM_SIZE);

    /* Reinitialize with defaults */
    g_StorageInitialized = false;
    return Storage_Init();
}

bool Storage_IsValid(void)
{
    StorageBlock_t block;

    if (!Storage_FlashRead(EEPROM_BASE_ADDRESS, (uint8_t *)&block, sizeof(StorageBlock_t)))
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

bool Storage_UpdateFrameCounters(uint32_t uplink, uint32_t downlink)
{
    if (!g_StorageInitialized)
    {
        return false;
    }

    g_StorageCache.FrameCounterUp = uplink;
    g_StorageCache.FrameCounterDown = downlink;
    return Storage_Save(&g_StorageCache);
}

bool Storage_UpdateJoinKeys(uint32_t devAddr, const uint8_t *nwkSKey, const uint8_t *appSKey)
{
    if (!g_StorageInitialized || nwkSKey == NULL || appSKey == NULL)
    {
        return false;
    }

    g_StorageCache.DevAddr = devAddr;
    memcpy(g_StorageCache.NwkSKey, nwkSKey, 16);
    memcpy(g_StorageCache.AppSKey, appSKey, 16);
    return Storage_Save(&g_StorageCache);
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static uint32_t Storage_CalculateCrc(const StorageData_t *data)
{
    /* Simple CRC32 calculation (use HAL CRC peripheral if available) */
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *ptr = (const uint8_t *)data;
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
    uint32_t offset = address - EEPROM_BASE_ADDRESS;
    if (address < EEPROM_BASE_ADDRESS || (offset + size) > EEPROM_SIZE)
    {
        return false;
    }
    uint8_t blank = 0xFF;

    for (uint32_t i = 0; i < size; i++)
    {
        if (EepromMcuWriteBuffer((uint16_t)(offset + i), &blank, 1) != LMN_STATUS_OK)
        {
            return false;
        }
    }
    return true;
}

static bool Storage_FlashWrite(uint32_t address, const uint8_t *data, uint32_t size)
{
    uint32_t offset = address - EEPROM_BASE_ADDRESS;
    if (address < EEPROM_BASE_ADDRESS || (offset + size) > EEPROM_SIZE)
    {
        return false;
    }
    return (EepromMcuWriteBuffer((uint16_t)offset, (uint8_t *)data, (uint16_t)size) == LMN_STATUS_OK);
}

static bool Storage_FlashRead(uint32_t address, uint8_t *data, uint32_t size)
{
    uint32_t offset = address - EEPROM_BASE_ADDRESS;
    if (address < EEPROM_BASE_ADDRESS || (offset + size) > EEPROM_SIZE)
    {
        return false;
    }
    return (EepromMcuReadBuffer((uint16_t)offset, data, (uint16_t)size) == LMN_STATUS_OK);
}
