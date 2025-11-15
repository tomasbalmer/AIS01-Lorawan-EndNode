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
#define OEM_STORAGE_OFFSET (EEPROM_BASE_ADDRESS + 0x0800U)

/* ============================================================================
 * PRIVATE TYPES
 * ========================================================================== */
typedef struct
{
    StorageHeader_t Header;
    StorageData_t Data;
} StorageBlock_t;

typedef struct __attribute__((packed))
{
    uint8_t DevEui[8];
    uint8_t AppEui[8];
    uint8_t AppKey[16];
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
    uint32_t DevAddr;
    uint32_t FrameCounterUp;
    uint32_t FrameCounterDown;
    uint32_t TxDutyCycle;
    uint32_t Rx1Delay;
    uint32_t Rx2Delay;
    uint32_t JoinRx1Delay;
    uint32_t JoinRx2Delay;
    uint32_t Rx2Frequency;
    uint8_t AdrEnabled;
    uint8_t DataRate;
    uint8_t TxPower;
    uint8_t Rx2DataRate;
    uint8_t FreqBand;
    uint8_t ConfirmedMsg;
    uint8_t AppPort;
    uint8_t JoinMode;
    uint8_t DisableFrameCounterCheck;
    uint8_t Reserved[14];
} OemStorageLayout_t;

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
static void Storage_SetDefaults(StorageData_t *data);
static bool Storage_BufferIsUniform(const uint8_t *buffer, uint32_t size, uint8_t value);
static bool Storage_ReadBlock(uint32_t address, StorageData_t *data);
static StorageStatus_t Storage_WriteBlockRaw(const StorageData_t *data);
static StorageStatus_t Storage_MigrateFromOem(StorageData_t *out);
static bool Storage_OemLayoutLooksValid(const OemStorageLayout_t *oem);

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
        Storage_SetDefaults(&g_StorageCache);

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

    StorageData_t temp;

    if (Storage_ReadBlock(STORAGE_PRIMARY_ADDRESS, &temp))
    {
        memcpy(data, &temp, sizeof(StorageData_t));
        return STORAGE_OK;
    }

    if (Storage_ReadBlock(STORAGE_BACKUP_ADDRESS, &temp))
    {
        memcpy(data, &temp, sizeof(StorageData_t));
        return STORAGE_RESTORED_FROM_BACKUP;
    }

    StorageStatus_t migrationStatus = Storage_MigrateFromOem(&temp);
    if (migrationStatus == STORAGE_OK)
    {
        memcpy(data, &temp, sizeof(StorageData_t));
        StorageStatus_t persistStatus = Storage_WriteBlockRaw(&temp);
        if (persistStatus != STORAGE_OK)
        {
            return persistStatus;
        }
        return STORAGE_OK;
    }

    return migrationStatus;
}

StorageStatus_t Storage_Save(const StorageData_t *data)
{
    if (data == NULL || !g_StorageInitialized)
    {
        return STORAGE_ERROR_PARAM;
    }

    StorageStatus_t status = Storage_WriteBlockRaw(data);
    if (status == STORAGE_OK && data != &g_StorageCache)
    {
        memcpy(&g_StorageCache, data, sizeof(StorageData_t));
    }

    return status;
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
    StorageData_t data;
    return Storage_ReadBlock(STORAGE_PRIMARY_ADDRESS, &data);
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
static void Storage_SetDefaults(StorageData_t *data)
{
    if (data == NULL)
    {
        return;
    }

    memset(data, 0, sizeof(StorageData_t));
    data->TxDutyCycle = LORAWAN_DEFAULT_TDC;
    data->AdrEnabled = (LORAWAN_DEFAULT_ADR_STATE != 0);
    data->DataRate = LORAWAN_DEFAULT_DATARATE;
    data->TxPower = LORAWAN_DEFAULT_TX_POWER;
    data->Rx2DataRate = LORAWAN_RX2_DATARATE;
    data->Rx2Frequency = LORAWAN_RX2_FREQUENCY;
    data->Rx1Delay = LORAWAN_RX1_DELAY;
    data->Rx2Delay = LORAWAN_RX2_DELAY;
    data->JoinRx1Delay = LORAWAN_JOIN_RX1_DELAY;
    data->JoinRx2Delay = LORAWAN_JOIN_RX2_DELAY;
    data->FreqBand = LORAWAN_AU915_SUB_BAND;
    data->DeviceClass = LORAWAN_DEFAULT_CLASS;
    data->ConfirmedMsg = (LORAWAN_DEFAULT_CONFIRMED_MSG != 0);
    data->AppPort = LORAWAN_DEFAULT_APP_PORT;
    data->FrameCounterUp = 0;
    data->FrameCounterDown = 0;
    data->JoinMode = 1U;
    data->DisableFrameCounterCheck = 0U;
    data->RetryCount = LORAWAN_DEFAULT_RETRY_COUNT;
    data->RetryDelay = LORAWAN_DEFAULT_RETRY_DELAY;
}

static bool Storage_BufferIsUniform(const uint8_t *buffer, uint32_t size, uint8_t value)
{
    if (buffer == NULL || size == 0U)
    {
        return true;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        if (buffer[i] != value)
        {
            return false;
        }
    }
    return true;
}

static bool Storage_ReadBlock(uint32_t address, StorageData_t *data)
{
    StorageHeader_t header;

    if (!Storage_FlashRead(address, (uint8_t *)&header, sizeof(StorageHeader_t)))
    {
        return false;
    }

    if ((header.Magic != STORAGE_MAGIC) ||
        (header.Version != STORAGE_VERSION) ||
        (header.Length != sizeof(StorageData_t)))
    {
        return false;
    }

    if (!Storage_FlashRead(address + sizeof(StorageHeader_t), (uint8_t *)data, sizeof(StorageData_t)))
    {
        return false;
    }

    return (Storage_CalculateCrc(data) == data->Crc);
}

static StorageStatus_t Storage_WriteBlockRaw(const StorageData_t *data)
{
    if (data == NULL)
    {
        return STORAGE_ERROR_PARAM;
    }

    StorageBlock_t block;
    StorageBlock_t verifyBlock;

    block.Header.Magic = STORAGE_MAGIC;
    block.Header.Version = STORAGE_VERSION;
    block.Header.Length = sizeof(StorageData_t);
    memcpy(&block.Data, data, sizeof(StorageData_t));
    block.Data.Crc = Storage_CalculateCrc(&block.Data);

    if (!Storage_FlashErase(STORAGE_BACKUP_ADDRESS, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_ERASE;
    }

    if (!Storage_FlashWrite(STORAGE_BACKUP_ADDRESS, (const uint8_t *)&block, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_WRITE;
    }

    if (!Storage_FlashRead(STORAGE_BACKUP_ADDRESS, (uint8_t *)&verifyBlock, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_VERIFY;
    }

    if (memcmp(&block, &verifyBlock, sizeof(StorageBlock_t)) != 0)
    {
        return STORAGE_ERROR_VERIFY;
    }

    if (!Storage_FlashErase(STORAGE_PRIMARY_ADDRESS, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_ERASE;
    }

    if (!Storage_FlashWrite(STORAGE_PRIMARY_ADDRESS, (const uint8_t *)&block, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_WRITE;
    }

    if (!Storage_FlashRead(STORAGE_PRIMARY_ADDRESS, (uint8_t *)&verifyBlock, sizeof(StorageBlock_t)))
    {
        return STORAGE_ERROR_VERIFY;
    }

    if (memcmp(&block, &verifyBlock, sizeof(StorageBlock_t)) != 0)
    {
        return STORAGE_ERROR_VERIFY;
    }

    return STORAGE_OK;
}

static bool Storage_OemLayoutLooksValid(const OemStorageLayout_t *oem)
{
    if (oem == NULL)
    {
        return false;
    }

    bool hasDevEui = !Storage_BufferIsUniform(oem->DevEui, sizeof(oem->DevEui), 0xFF);
    bool hasKeys = !Storage_BufferIsUniform(oem->AppKey, sizeof(oem->AppKey), 0xFF);

    if (!hasDevEui && !hasKeys)
    {
        return false;
    }

    if (oem->JoinMode > 1U)
    {
        return false;
    }

    if (oem->AdrEnabled > 1U)
    {
        return false;
    }

    if (oem->TxDutyCycle == 0xFFFFFFFFU)
    {
        return false;
    }

    return true;
}

static StorageStatus_t Storage_MigrateFromOem(StorageData_t *out)
{
    if (out == NULL)
    {
        return STORAGE_ERROR_PARAM;
    }

    OemStorageLayout_t raw;
    if (EepromMcuReadBuffer(OEM_STORAGE_OFFSET, (uint8_t *)&raw, sizeof(raw)) != LMN_STATUS_OK)
    {
        return STORAGE_ERROR_READ;
    }

    if (Storage_BufferIsUniform((const uint8_t *)&raw, sizeof(raw), 0xFF))
    {
        return STORAGE_ERROR_INIT;
    }

    if (!Storage_OemLayoutLooksValid(&raw))
    {
        return STORAGE_ERROR_INIT;
    }

    StorageData_t migrated;
    Storage_SetDefaults(&migrated);

    if (!Storage_BufferIsUniform(raw.DevEui, sizeof(raw.DevEui), 0xFF))
    {
        memcpy(migrated.DevEui, raw.DevEui, sizeof(raw.DevEui));
    }

    if (!Storage_BufferIsUniform(raw.AppEui, sizeof(raw.AppEui), 0xFF))
    {
        memcpy(migrated.AppEui, raw.AppEui, sizeof(raw.AppEui));
    }

    if (!Storage_BufferIsUniform(raw.AppKey, sizeof(raw.AppKey), 0xFF))
    {
        memcpy(migrated.AppKey, raw.AppKey, sizeof(raw.AppKey));
    }

    if (!Storage_BufferIsUniform(raw.NwkSKey, sizeof(raw.NwkSKey), 0xFF))
    {
        memcpy(migrated.NwkSKey, raw.NwkSKey, sizeof(raw.NwkSKey));
    }

    if (!Storage_BufferIsUniform(raw.AppSKey, sizeof(raw.AppSKey), 0xFF))
    {
        memcpy(migrated.AppSKey, raw.AppSKey, sizeof(raw.AppSKey));
    }

    if (raw.FrameCounterUp != 0xFFFFFFFFU)
    {
        memcpy(&migrated.FrameCounterUp, &raw.FrameCounterUp, sizeof(uint32_t));
    }

    if (raw.FrameCounterDown != 0xFFFFFFFFU)
    {
        memcpy(&migrated.FrameCounterDown, &raw.FrameCounterDown, sizeof(uint32_t));
    }

    if (raw.TxDutyCycle != 0xFFFFFFFFU)
    {
        memcpy(&migrated.TxDutyCycle, &raw.TxDutyCycle, sizeof(uint32_t));
    }

    if (raw.Rx1Delay != 0xFFFFFFFFU)
    {
        memcpy(&migrated.Rx1Delay, &raw.Rx1Delay, sizeof(uint32_t));
    }

    if (raw.Rx2Delay != 0xFFFFFFFFU)
    {
        memcpy(&migrated.Rx2Delay, &raw.Rx2Delay, sizeof(uint32_t));
    }

    if (raw.JoinRx1Delay != 0xFFFFFFFFU)
    {
        memcpy(&migrated.JoinRx1Delay, &raw.JoinRx1Delay, sizeof(uint32_t));
    }

    if (raw.JoinRx2Delay != 0xFFFFFFFFU)
    {
        memcpy(&migrated.JoinRx2Delay, &raw.JoinRx2Delay, sizeof(uint32_t));
    }

    if (raw.Rx2Frequency >= 100000000U && raw.Rx2Frequency <= 1000000000U)
    {
        memcpy(&migrated.Rx2Frequency, &raw.Rx2Frequency, sizeof(uint32_t));
    }

    if (raw.DevAddr != 0xFFFFFFFFU)
    {
        memcpy(&migrated.DevAddr, &raw.DevAddr, sizeof(uint32_t));
    }

    if (raw.DataRate != 0xFFU)
    {
        migrated.DataRate = (raw.DataRate & 0x0F);
    }

    if (raw.TxPower != 0xFFU)
    {
        migrated.TxPower = (raw.TxPower & 0x0F);
    }

    if (raw.Rx2DataRate != 0xFFU)
    {
        migrated.Rx2DataRate = (raw.Rx2DataRate & 0x0F);
    }

    if (raw.FreqBand != 0xFFU)
    {
        migrated.FreqBand = (raw.FreqBand != 0U) ? 1U : 0U;
    }

    if (raw.AppPort != 0xFFU)
    {
        migrated.AppPort = raw.AppPort;
    }

    if (raw.JoinMode <= 1U)
    {
        migrated.JoinMode = (raw.JoinMode != 0U) ? 1U : 0U;
    }

    if (raw.DisableFrameCounterCheck <= 1U)
    {
        migrated.DisableFrameCounterCheck =
            (raw.DisableFrameCounterCheck != 0U) ? 1U : 0U;
    }

    if (raw.ConfirmedMsg <= 1U)
    {
        migrated.ConfirmedMsg = (raw.ConfirmedMsg != 0U) ? 1U : 0U;
    }

    if (raw.AdrEnabled <= 1U)
    {
        migrated.AdrEnabled = (raw.AdrEnabled != 0U) ? 1U : 0U;
    }

    memcpy(out, &migrated, sizeof(StorageData_t));
    return STORAGE_OK;
}

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
