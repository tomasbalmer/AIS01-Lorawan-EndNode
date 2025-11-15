/*!
 * \file      storage.h
 *
 * \brief     Non-volatile storage management (EEPROM emulation in Flash)
 *
 * \details   Provides persistent storage for LoRaWAN credentials and configuration.
 *            Uses flash memory to emulate EEPROM for STM32L072CZ.
 */
#ifndef __STORAGE_H__
#define __STORAGE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#define STORAGE_MAGIC (0x41595301UL)
#define STORAGE_VERSION (1U)

    typedef struct
    {
        uint32_t Magic;
        uint16_t Version;
        uint16_t Length;
    } StorageHeader_t;

    /* ============================================================================
     * STORAGE STATUS CODES
     * ========================================================================== */
    typedef enum
    {
        STORAGE_OK = 0,              /* Operation successful */
        STORAGE_ERROR_INIT,          /* Initialization failed */
        STORAGE_ERROR_READ,          /* Read operation failed */
        STORAGE_ERROR_WRITE,         /* Write operation failed */
        STORAGE_ERROR_ERASE,         /* Erase operation failed */
        STORAGE_ERROR_VERIFY,        /* Verification failed */
        STORAGE_ERROR_CRC,           /* CRC mismatch (data corrupted) */
        STORAGE_ERROR_MAGIC,         /* Magic number invalid */
        STORAGE_ERROR_VERSION,       /* Version mismatch */
        STORAGE_ERROR_PARAM,         /* Invalid parameter */
        STORAGE_FACTORY_RESET,       /* Factory reset performed (both copies corrupted) */
        STORAGE_RESTORED_FROM_BACKUP /* Restored from backup copy */
    } StorageStatus_t;

    /* ============================================================================
     * STORAGE KEY DEFINITIONS
     * ========================================================================== */
    typedef enum
    {
        STORAGE_KEY_DEVEUI = 0,   /* DevEUI (8 bytes) */
        STORAGE_KEY_APPEUI,       /* AppEUI (8 bytes) */
        STORAGE_KEY_APPKEY,       /* AppKey (16 bytes) */
        STORAGE_KEY_DEVADDR,      /* DevAddr (4 bytes) */
        STORAGE_KEY_NWKSKEY,      /* NwkSKey (16 bytes) */
        STORAGE_KEY_APPSKEY,      /* AppSKey (16 bytes) */
        STORAGE_KEY_TDC,          /* Transmission Duty Cycle (4 bytes) */
        STORAGE_KEY_ADR,          /* ADR Enable (1 byte) */
        STORAGE_KEY_DR,           /* Data Rate (1 byte) */
        STORAGE_KEY_TXP,          /* TX Power (1 byte) */
        STORAGE_KEY_RX2DR,        /* RX2 Data Rate (1 byte) */
        STORAGE_KEY_RX2FQ,        /* RX2 Frequency (4 bytes) */
        STORAGE_KEY_RX1DL,        /* RX1 Delay (4 bytes) */
        STORAGE_KEY_RX2DL,        /* RX2 Delay (4 bytes) */
        STORAGE_KEY_JRX1DL,       /* Join RX1 Delay (4 bytes) */
        STORAGE_KEY_JRX2DL,       /* Join RX2 Delay (4 bytes) */
        STORAGE_KEY_FREQBAND,     /* Frequency Sub-band (1 byte) */
        STORAGE_KEY_CLASS,        /* Device Class (1 byte) */
        STORAGE_KEY_CONFIRMED,    /* Confirmed messages (1 byte) */
        STORAGE_KEY_PORT,         /* Application Port (1 byte) */
        STORAGE_KEY_FCNTUP,       /* Uplink frame counter (4 bytes) */
        STORAGE_KEY_FCNTDOWN,     /* Downlink frame counter (4 bytes) */
        STORAGE_KEY_JOIN_MODE,    /* Join mode: 0=ABP, 1=OTAA (1 byte) */
        STORAGE_KEY_DISABLE_FCNT, /* Disable frame counter check (1 byte) */
        STORAGE_KEY_RETRY,        /* Confirmed message retry count (1 byte) */
        STORAGE_KEY_RETRY_DELAY,  /* Delay between retries (4 bytes) */
        STORAGE_KEY_CALIBRATION,  /* Calibration Data (variable) */
        STORAGE_KEY_MAX
    } StorageKey_t;

    /* ============================================================================
     * STORAGE DATA STRUCTURE
     * ========================================================================== */
    typedef struct
    {
        uint8_t DevEui[8];
        uint8_t AppEui[8];
        uint8_t AppKey[16];
        uint32_t DevAddr;
        uint8_t NwkSKey[16];
        uint8_t AppSKey[16];
        uint32_t TxDutyCycle;
        uint8_t AdrEnabled;
        uint8_t DataRate;
        uint8_t TxPower;
        uint8_t Rx2DataRate;
        uint32_t Rx2Frequency;
        uint32_t Rx1Delay;
        uint32_t Rx2Delay;
        uint32_t JoinRx1Delay;
        uint32_t JoinRx2Delay;
        uint8_t FreqBand;
        uint8_t DeviceClass;
        uint8_t ConfirmedMsg;
        uint8_t AppPort;
        uint32_t FrameCounterUp;
        uint32_t FrameCounterDown;
        uint8_t JoinMode;                 /* Join mode: 0=ABP, 1=OTAA */
        uint8_t DisableFrameCounterCheck; /* Disable frame counter check for testing */
        uint8_t RetryCount;               /* Confirmed message retry count */
        uint32_t RetryDelay;              /* Delay between retries (ms) */
        uint8_t CalibrationData[32];      /* Reserved for calibration parameters */
        uint32_t Crc;                     /* CRC32 for data integrity */
    } StorageData_t;

    /* ============================================================================
     * PUBLIC FUNCTION PROTOTYPES
     * ========================================================================== */

    /*!
     * \brief Initializes the storage system
     * \retval STORAGE_OK if successful
     * \retval STORAGE_ERROR_INIT if initialization failed
     * \retval STORAGE_FACTORY_RESET if both primary and backup invalid (factory reset applied)
     * \retval STORAGE_RESTORED_FROM_BACKUP if primary corrupted, restored from backup
     */
    StorageStatus_t Storage_Init(void);

    /*!
     * \brief Loads configuration from non-volatile storage
     * \param [out] data Pointer to storage data structure
     * \retval STORAGE_OK if data loaded successfully
     * \retval STORAGE_ERROR_PARAM if data pointer is NULL
     * \retval STORAGE_ERROR_READ if read operation failed
     * \retval STORAGE_ERROR_CRC if CRC validation failed
     */
    StorageStatus_t Storage_Load(StorageData_t *data);

    /*!
     * \brief Saves configuration to non-volatile storage
     * \param [in] data Pointer to storage data structure
     * \retval STORAGE_OK if data saved successfully
     * \retval STORAGE_ERROR_PARAM if data pointer is NULL
     * \retval STORAGE_ERROR_WRITE if write operation failed
     * \retval STORAGE_ERROR_VERIFY if verification failed
     */
    StorageStatus_t Storage_Save(const StorageData_t *data);

    /*!
     * \brief Reads a specific key from storage
     * \param [in] key Storage key identifier
     * \param [out] buffer Buffer to store read data
     * \param [in] size Size of buffer
     * \retval STORAGE_OK if read successful
     * \retval STORAGE_ERROR_PARAM if invalid parameters
     * \retval STORAGE_ERROR_READ if read failed
     */
    StorageStatus_t Storage_Read(StorageKey_t key, uint8_t *buffer, uint32_t size);

    /*!
     * \brief Writes a specific key to storage
     * \param [in] key Storage key identifier
     * \param [in] buffer Buffer containing data to write
     * \param [in] size Size of data
     * \retval STORAGE_OK if write successful
     * \retval STORAGE_ERROR_PARAM if invalid parameters
     * \retval STORAGE_ERROR_WRITE if write failed
     */
    StorageStatus_t Storage_Write(StorageKey_t key, const uint8_t *buffer, uint32_t size);

    /*!
     * \brief Performs factory reset (erases all stored data)
     * \retval STORAGE_OK if reset successful
     * \retval STORAGE_ERROR_ERASE if erase failed
     */
    StorageStatus_t Storage_FactoryReset(void);

    /*!
     * \brief Checks if storage contains valid data
     * \retval true if valid data exists, false otherwise
     */
    bool Storage_IsValid(void);

    /*!
     * \brief Updates frame counters (uplink and downlink)
     * \param [in] uplink Uplink frame counter
     * \param [in] downlink Downlink frame counter
     * \retval STORAGE_OK if updated successfully
     */
    StorageStatus_t Storage_UpdateFrameCounters(uint32_t uplink, uint32_t downlink);

    /*!
     * \brief Updates join session keys after OTAA join
     * \param [in] devAddr Device address
     * \param [in] nwkSKey Network session key (16 bytes)
     * \param [in] appSKey Application session key (16 bytes)
     * \retval STORAGE_OK if updated successfully
     */
    StorageStatus_t Storage_UpdateJoinKeys(uint32_t devAddr, const uint8_t *nwkSKey, const uint8_t *appSKey);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
