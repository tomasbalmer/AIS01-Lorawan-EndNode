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

StorageStatus_t Storage_Init(void)
{
    if (g_StorageInitialized)
    {
        return STORAGE_OK;
    }

    StorageStatus_t status = Storage_Load(&g_StorageCache);

    if (status == STORAGE_OK)
    {
        /* Primary storage is valid, initialization successful */
        g_StorageInitialized = true;
        return STORAGE_OK;
    }
    else if (status == STORAGE_RESTORED_FROM_BACKUP)
    {
        /* Backup was valid, primary was corrupted (Phase 2.5) */
        g_StorageInitialized = true;

        /* Restore primary from backup - write the loaded data back */
        StorageStatus_t saveStatus = Storage_Save(&g_StorageCache);
        if (saveStatus != STORAGE_OK)
        {
            /* Failed to restore primary, but we have valid data from backup */
            /* Continue anyway - next save will attempt to restore primary */
        }

        /* Return backup restore status to inform caller */
        return STORAGE_RESTORED_FROM_BACKUP;
    }
    else
    {
        /* Both primary and backup failed - initialize with defaults and perform factory reset */
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
        g_StorageCache.JoinRx1Delay = LORAWAN_JOIN_RX1_DELAY;
        g_StorageCache.JoinRx2Delay = LORAWAN_JOIN_RX2_DELAY;
        g_StorageCache.FreqBand = LORAWAN_AU915_SUB_BAND;
        g_StorageCache.DeviceClass = LORAWAN_DEFAULT_CLASS;
        g_StorageCache.ConfirmedMsg = (LORAWAN_DEFAULT_CONFIRMED_MSG != 0);
        g_StorageCache.AppPort = LORAWAN_DEFAULT_APP_PORT;
        g_StorageCache.FrameCounterUp = 0;
        g_StorageCache.FrameCounterDown = 0;
        g_StorageCache.JoinMode = 1;                 /* Default: OTAA */
        g_StorageCache.DisableFrameCounterCheck = 0; /* Default: frame counter check enabled */
        g_StorageCache.RetryCount = LORAWAN_DEFAULT_RETRY_COUNT;
        g_StorageCache.RetryDelay = LORAWAN_DEFAULT_RETRY_DELAY;

        g_StorageInitialized = true;

        /* Attempt to save defaults to both primary and backup */
        StorageStatus_t saveStatus = Storage_Save(&g_StorageCache);
        if (saveStatus != STORAGE_OK)
        {
            g_StorageInitialized = false;
            return STORAGE_ERROR_INIT;
        }

        /* Return factory reset status to inform caller */
        return STORAGE_FACTORY_RESET;
    }
}

StorageStatus_t Storage_Load(StorageData_t *data)
{
    if (data == NULL)
    {
        return STORAGE_ERROR_PARAM;
    }

    StorageBlock_t primaryBlock;
    StorageBlock_t backupBlock;
    bool primaryValid = false;
    bool backupValid = false;

    /* ========================================================================
     * STEP 1: Try to read and validate PRIMARY copy
     * ====================================================================== */
    if (Storage_FlashRead(STORAGE_PRIMARY_ADDRESS, (uint8_t *)&primaryBlock, sizeof(StorageBlock_t)))
    {
        if (primaryBlock.Magic == STORAGE_MAGIC &&
            primaryBlock.Version == STORAGE_VERSION)
        {
            uint32_t calculatedCrc = Storage_CalculateCrc(&primaryBlock.Data);
            if (calculatedCrc == primaryBlock.Data.Crc)
            {
                primaryValid = true;
            }
        }
    }

    /* If primary is valid, use it immediately */
    if (primaryValid)
    {
        memcpy(data, &primaryBlock.Data, sizeof(StorageData_t));
        return STORAGE_OK;
    }

    /* ========================================================================
     * STEP 2: Primary failed, try BACKUP copy
     * ====================================================================== */
    if (Storage_FlashRead(STORAGE_BACKUP_ADDRESS, (uint8_t *)&backupBlock, sizeof(StorageBlock_t)))
    {
        if (backupBlock.Magic == STORAGE_MAGIC &&
            backupBlock.Version == STORAGE_VERSION)
        {
            uint32_t calculatedCrc = Storage_CalculateCrc(&backupBlock.Data);
            if (calculatedCrc == backupBlock.Data.Crc)
            {
                backupValid = true;
            }
        }
    }

    /* If backup is valid, restore from it */
    if (backupValid)
    {
        memcpy(data, &backupBlock.Data, sizeof(StorageData_t));

        /* Optionally restore primary from backup (write-through) */
        /* Note: We'll do this in Storage_Init() to avoid nested saves */

        return STORAGE_RESTORED_FROM_BACKUP;
    }

    /* Return appropriate error based on what we found */
    if (!primaryValid && !backupValid)
    {
        return STORAGE_ERROR_CRC; /* Both corrupted or uninitialized */
    }

    return STORAGE_ERROR_READ;
}

StorageStatus_t Storage_Save(const StorageData_t *data)
{
    if (data == NULL || !g_StorageInitialized)
    {
        return STORAGE_ERROR_PARAM;
    }

    StorageBlock_t block;
    StorageBlock_t verifyBlock;

    /* Prepare block */
    block.Magic = STORAGE_MAGIC;
    block.Version = STORAGE_VERSION;
    memcpy(&block.Data, data, sizeof(StorageData_t));

    /* Calculate CRC */
    block.Data.Crc = Storage_CalculateCrc(&block.Data);

    /* Erase backup region */
    if (!Storage_FlashErase(STORAGE_BACKUP_ADDRESS, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_ERASE;
    }

    /* Write to backup */
    if (!Storage_FlashWrite(STORAGE_BACKUP_ADDRESS, (const uint8_t *)&block, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_WRITE;
    }

    /* Verify backup write */
    if (!Storage_FlashRead(STORAGE_BACKUP_ADDRESS, (uint8_t *)&verifyBlock, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_VERIFY;
    }

    if (memcmp(&block, &verifyBlock, sizeof(StorageBlock_t)) != 0)
    {
        return STORAGE_ERROR_VERIFY;
    }

    /* Erase primary region */
    if (!Storage_FlashErase(STORAGE_PRIMARY_ADDRESS, sizeof(StorageBlock_t)))
    {
        /* Backup is written, primary erase failed - still recoverable */
        return STORAGE_ERROR_ERASE;
    }

    /* Write to primary */
    if (!Storage_FlashWrite(STORAGE_PRIMARY_ADDRESS, (const uint8_t *)&block, sizeof(StorageBlock_t)))
    {
        /* Primary write failed, but backup is valid - still recoverable */
        return STORAGE_ERROR_WRITE;
    }

    /* Verify primary write */
    if (!Storage_FlashRead(STORAGE_PRIMARY_ADDRESS, (uint8_t *)&verifyBlock, sizeof(StorageBlock_t)))
    {
        /* Primary verify failed, but backup is valid - still recoverable */
        return STORAGE_ERROR_VERIFY;
    }

    if (memcmp(&block, &verifyBlock, sizeof(StorageBlock_t)) != 0)
    {
        /* Primary verify failed, but backup is valid - still recoverable */
        return STORAGE_ERROR_VERIFY;
    }

    memcpy(&g_StorageCache, data, sizeof(StorageData_t));

    return STORAGE_OK;
}

StorageStatus_t Storage_Read(StorageKey_t key, uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || !g_StorageInitialized)
    {
        return STORAGE_ERROR_PARAM;
    }

    /* Read from cache */
    switch (key)
    {
    case STORAGE_KEY_DEVEUI:
        if (size >= 8)
            memcpy(buffer, g_StorageCache.DevEui, 8);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_APPEUI:
        if (size >= 8)
            memcpy(buffer, g_StorageCache.AppEui, 8);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_APPKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.AppKey, 16);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_DEVADDR:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.DevAddr, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_NWKSKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.NwkSKey, 16);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_APPSKEY:
        if (size >= 16)
            memcpy(buffer, g_StorageCache.AppSKey, 16);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_TDC:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.TxDutyCycle, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_ADR:
        if (size >= 1)
            *buffer = g_StorageCache.AdrEnabled;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_DR:
        if (size >= 1)
            *buffer = g_StorageCache.DataRate;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_TXP:
        if (size >= 1)
            *buffer = g_StorageCache.TxPower;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RX2DR:
        if (size >= 1)
            *buffer = g_StorageCache.Rx2DataRate;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RX2FQ:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx2Frequency, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RX1DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx1Delay, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RX2DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.Rx2Delay, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_JRX1DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.JoinRx1Delay, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_JRX2DL:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.JoinRx2Delay, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_FREQBAND:
        if (size >= 1)
            *buffer = g_StorageCache.FreqBand;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_CLASS:
        if (size >= 1)
            *buffer = g_StorageCache.DeviceClass;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_CONFIRMED:
        if (size >= 1)
            *buffer = g_StorageCache.ConfirmedMsg;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_PORT:
        if (size >= 1)
            *buffer = g_StorageCache.AppPort;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_FCNTUP:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.FrameCounterUp, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_FCNTDOWN:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.FrameCounterDown, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_JOIN_MODE:
        if (size >= 1)
            *buffer = g_StorageCache.JoinMode;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_DISABLE_FCNT:
        if (size >= 1)
            *buffer = g_StorageCache.DisableFrameCounterCheck;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RETRY:
        if (size >= 1)
            *buffer = g_StorageCache.RetryCount;
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_RETRY_DELAY:
        if (size >= 4)
            memcpy(buffer, &g_StorageCache.RetryDelay, 4);
        else
            return STORAGE_ERROR_PARAM;
        break;

    case STORAGE_KEY_CALIBRATION:
        if (size >= sizeof(g_StorageCache.CalibrationData))
        {
            memcpy(buffer, g_StorageCache.CalibrationData, sizeof(g_StorageCache.CalibrationData));
        }
        else
            return STORAGE_ERROR_PARAM;
        break;

    default:
        return STORAGE_ERROR_PARAM;
    }

    return STORAGE_OK;
}

StorageStatus_t Storage_Write(StorageKey_t key, const uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL || !g_StorageInitialized)
    {
        return STORAGE_ERROR_PARAM;
    }

    switch (key)
    {
    case STORAGE_KEY_DEVEUI:
        if (size != 8)
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.DevEui, buffer, 8);
        break;

    case STORAGE_KEY_APPEUI:
        if (size != 8)
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.AppEui, buffer, 8);
        break;

    case STORAGE_KEY_APPKEY:
        if (size != 16)
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.AppKey, buffer, 16);
        break;

    case STORAGE_KEY_DEVADDR:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.DevAddr, buffer, 4);
        break;

    case STORAGE_KEY_NWKSKEY:
        if (size != 16)
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.NwkSKey, buffer, 16);
        break;

    case STORAGE_KEY_APPSKEY:
        if (size != 16)
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.AppSKey, buffer, 16);
        break;

    case STORAGE_KEY_TDC:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.TxDutyCycle, buffer, 4);
        break;

    case STORAGE_KEY_ADR:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.AdrEnabled = *buffer;
        break;

    case STORAGE_KEY_DR:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.DataRate = *buffer;
        break;

    case STORAGE_KEY_TXP:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.TxPower = *buffer;
        break;

    case STORAGE_KEY_RX2DR:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.Rx2DataRate = *buffer;
        break;

    case STORAGE_KEY_RX2FQ:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.Rx2Frequency, buffer, 4);
        break;

    case STORAGE_KEY_RX1DL:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.Rx1Delay, buffer, 4);
        break;

    case STORAGE_KEY_RX2DL:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.Rx2Delay, buffer, 4);
        break;

    case STORAGE_KEY_JRX1DL:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.JoinRx1Delay, buffer, 4);
        break;

    case STORAGE_KEY_JRX2DL:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.JoinRx2Delay, buffer, 4);
        break;

    case STORAGE_KEY_FREQBAND:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.FreqBand = *buffer;
        break;

    case STORAGE_KEY_CLASS:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.DeviceClass = *buffer;
        break;

    case STORAGE_KEY_CONFIRMED:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.ConfirmedMsg = *buffer;
        break;

    case STORAGE_KEY_PORT:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.AppPort = *buffer;
        break;

    case STORAGE_KEY_FCNTUP:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.FrameCounterUp, buffer, 4);
        break;

    case STORAGE_KEY_FCNTDOWN:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.FrameCounterDown, buffer, 4);
        break;

    case STORAGE_KEY_JOIN_MODE:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.JoinMode = *buffer;
        break;

    case STORAGE_KEY_DISABLE_FCNT:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.DisableFrameCounterCheck = *buffer;
        break;

    case STORAGE_KEY_RETRY:
        if (size != 1)
            return STORAGE_ERROR_PARAM;
        g_StorageCache.RetryCount = *buffer;
        break;

    case STORAGE_KEY_RETRY_DELAY:
        if (size != 4)
            return STORAGE_ERROR_PARAM;
        memcpy(&g_StorageCache.RetryDelay, buffer, 4);
        break;

    case STORAGE_KEY_CALIBRATION:
        if (size > sizeof(g_StorageCache.CalibrationData))
            return STORAGE_ERROR_PARAM;
        memcpy(g_StorageCache.CalibrationData, buffer, size);
        break;

    default:
        return STORAGE_ERROR_PARAM;
    }

    return Storage_Save(&g_StorageCache);
}

StorageStatus_t Storage_FactoryReset(void)
{
    if (!g_StorageInitialized)
    {
        return STORAGE_ERROR_PARAM;
    }

    if (!Storage_FlashErase(EEPROM_BASE_ADDRESS, EEPROM_SIZE))
    {
        return STORAGE_ERROR_ERASE;
    }

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

StorageStatus_t Storage_UpdateFrameCounters(uint32_t uplink, uint32_t downlink)
{
    if (!g_StorageInitialized)
    {
        return STORAGE_ERROR_INIT;
    }

    g_StorageCache.FrameCounterUp = uplink;
    g_StorageCache.FrameCounterDown = downlink;
    return Storage_Save(&g_StorageCache);
}

StorageStatus_t Storage_UpdateJoinKeys(uint32_t devAddr, const uint8_t *nwkSKey, const uint8_t *appSKey)
{
    if (!g_StorageInitialized || nwkSKey == NULL || appSKey == NULL)
    {
        return STORAGE_ERROR_PARAM;
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
