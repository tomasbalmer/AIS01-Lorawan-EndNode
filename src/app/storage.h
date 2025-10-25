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

/* ============================================================================
 * STORAGE KEY DEFINITIONS
 * ========================================================================== */
typedef enum
{
    STORAGE_KEY_DEVEUI = 0,           /* DevEUI (8 bytes) */
    STORAGE_KEY_APPEUI,               /* AppEUI (8 bytes) */
    STORAGE_KEY_APPKEY,               /* AppKey (16 bytes) */
    STORAGE_KEY_TDC,                  /* Transmission Duty Cycle (4 bytes) */
    STORAGE_KEY_ADR,                  /* ADR Enable (1 byte) */
    STORAGE_KEY_DR,                   /* Data Rate (1 byte) */
    STORAGE_KEY_TXP,                  /* TX Power (1 byte) */
    STORAGE_KEY_RX2DR,                /* RX2 Data Rate (1 byte) */
    STORAGE_KEY_RX2FQ,                /* RX2 Frequency (4 bytes) */
    STORAGE_KEY_RX1DL,                /* RX1 Delay (4 bytes) */
    STORAGE_KEY_RX2DL,                /* RX2 Delay (4 bytes) */
    STORAGE_KEY_FREQBAND,             /* Frequency Sub-band (1 byte) */
    STORAGE_KEY_CLASS,                /* Device Class (1 byte) */
    STORAGE_KEY_CONFIRMED,            /* Confirmed messages (1 byte) */
    STORAGE_KEY_PORT,                 /* Application Port (1 byte) */
    STORAGE_KEY_CALIBRATION,          /* Calibration Data (variable) */
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
    uint32_t TxDutyCycle;
    uint8_t AdrEnabled;
    uint8_t DataRate;
    uint8_t TxPower;
    uint8_t Rx2DataRate;
    uint32_t Rx2Frequency;
    uint32_t Rx1Delay;
    uint32_t Rx2Delay;
    uint8_t FreqBand;
    uint8_t DeviceClass;
    uint8_t ConfirmedMsg;
    uint8_t AppPort;
    uint8_t CalibrationData[32];      /* Reserved for calibration parameters */
    uint32_t Crc;                     /* CRC32 for data integrity */
} StorageData_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ========================================================================== */

/*!
 * \brief Initializes the storage system
 * \retval true if initialization successful, false otherwise
 */
bool Storage_Init(void);

/*!
 * \brief Loads configuration from non-volatile storage
 * \param [out] data Pointer to storage data structure
 * \retval true if data loaded successfully, false otherwise
 */
bool Storage_Load(StorageData_t *data);

/*!
 * \brief Saves configuration to non-volatile storage
 * \param [in] data Pointer to storage data structure
 * \retval true if data saved successfully, false otherwise
 */
bool Storage_Save(const StorageData_t *data);

/*!
 * \brief Reads a specific key from storage
 * \param [in] key Storage key identifier
 * \param [out] buffer Buffer to store read data
 * \param [in] size Size of buffer
 * \retval true if read successful, false otherwise
 */
bool Storage_Read(StorageKey_t key, uint8_t *buffer, uint32_t size);

/*!
 * \brief Writes a specific key to storage
 * \param [in] key Storage key identifier
 * \param [in] buffer Buffer containing data to write
 * \param [in] size Size of data
 * \retval true if write successful, false otherwise
 */
bool Storage_Write(StorageKey_t key, const uint8_t *buffer, uint32_t size);

/*!
 * \brief Performs factory reset (erases all stored data)
 * \retval true if reset successful, false otherwise
 */
bool Storage_FactoryReset(void);

/*!
 * \brief Checks if storage contains valid data
 * \retval true if valid data exists, false otherwise
 */
bool Storage_IsValid(void);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */
