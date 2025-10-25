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
    CALIB_CMD_RESET = 0x01,           /* Reset calibration to defaults */
    CALIB_CMD_SET_OFFSET = 0x02,      /* Set sensor offset */
    CALIB_CMD_SET_GAIN = 0x03,        /* Set sensor gain */
    CALIB_CMD_SET_THRESHOLD = 0x04,   /* Set detection threshold */
    CALIB_CMD_QUERY = 0x05,           /* Query current calibration */
} CalibrationCmd_t;

/* ============================================================================
 * CALIBRATION DATA STRUCTURE
 * ========================================================================== */
typedef struct
{
    float Offset;                     /* Sensor offset value */
    float Gain;                       /* Sensor gain multiplier */
    float Threshold;                  /* Detection threshold */
    uint32_t Timestamp;               /* Last calibration timestamp */
    uint8_t Version;                  /* Calibration version */
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
bool Calibration_ProcessATCommand(const uint8_t *payload, uint8_t size);

/*!
 * \brief Gets current calibration data
 * \param [out] data Pointer to calibration data structure
 * \retval true if data retrieved successfully
 */
bool Calibration_GetData(CalibrationData_t *data);

/*!
 * \brief Sets calibration data
 * \param [in] data Pointer to calibration data structure
 * \retval true if data set successfully
 */
bool Calibration_SetData(const CalibrationData_t *data);

/*!
 * \brief Resets calibration to factory defaults
 * \retval true if reset successful
 */
bool Calibration_Reset(void);

/*!
 * \brief Applies calibration to raw sensor value
 * \param [in] rawValue Raw sensor reading
 * \retval Calibrated sensor value
 */
float Calibration_ApplyToValue(float rawValue);

#ifdef __cplusplus
}
#endif

#endif /* __CALIBRATION_H__ */
