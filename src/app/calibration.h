/*!
 * \file      calibration.h
 *
 * \brief     Remote calibration module
 *
 * \details   Handles remote calibration via AT commands and LoRaWAN downlinks.
 *            Allows adjustment of sensor parameters without physical access.
 */
#ifndef __CALIBRATION_H__
#define __CALIBRATION_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * CALIBRATION COMMAND TYPES
 * ========================================================================== */
typedef enum
{
    CALIB_OPCODE_QUERY = 0x00U,  /* Read current configuration */
    CALIB_OPCODE_APPLY = 0x01U    /* Apply configuration payload */
} CalibrationOpcode_t;

/* ============================================================================
 * CALIBRATION DATA STRUCTURE
 * ========================================================================== */
#define CALIBRATION_BUFFER_SIZE 32U

typedef struct
{
    uint8_t Command;                  /* Last opcode processed */
    uint8_t Flags;                    /* Raw flag field from payload */
    uint32_t Parameter;               /* Auxiliary parameter (channel/index) */
    uint32_t Value;                   /* Configuration value */
    uint32_t ApplyFlag;               /* Mirror of apply flag */
    uint8_t PendingSlots[3];          /* Software shadow of pending/apply bitfields */
    bool Busy;                        /* True while hardware apply in progress */
} CalibrationData_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ========================================================================== */

/*!
 * \brief Initializes calibration module
 * \retval true if initialization successful
 */
bool Calibration_Init(void);

/*!
 * \brief Processes a calibration command from downlink
 * \param [in] payload Downlink payload containing calibration data
 * \param [in] size Size of payload
 * \param [out] response Buffer for response payload
 * \param [out] responseSize Size of response payload
 * \retval true if command processed successfully
 */
bool Calibration_ProcessDownlink(const uint8_t *payload, uint8_t size,
                                  uint8_t *response, uint8_t *responseSize);

/*!
 * \brief Processes a calibration AT command
 * \param [in] payload Command payload
 * \param [in] size Size of payload
 * \retval true if command processed successfully
 */
/*!
 * \brief Gets the last processed calibration snapshot
 * \param [out] data Pointer that receives the snapshot
 * \retval true if data available
 */
bool Calibration_GetData(CalibrationData_t *data);

/*!
 * \brief Clears calibration context to defaults
 * \retval true when context cleared
 */
bool Calibration_Reset(void);

/*!
 * \brief Hardware wait helper (overridden by board layer)
 */
bool Calibration_HwWaitReady(void);

/*!
 * \brief Hardware enable helper (overridden by board layer)
 */
void Calibration_HwSetEnable(bool enable);

/*!
 * \brief Indicates whether a calibration apply is in progress
 */
bool Calibration_IsBusy(void);

/*!
 * \brief Indicates whether there is any pending apply flag recorded
 */
bool Calibration_HasPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __CALIBRATION_H__ */
