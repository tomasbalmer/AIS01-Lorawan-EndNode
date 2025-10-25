/*!
 * \file      calibration.c
 *
 * \brief     Remote calibration implementation
 *
 * \details   Handles remote calibration commands and stores calibration data
 *            in non-volatile storage.
 */
#include "calibration.h"
#include "storage.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static CalibrationData_t g_CalibrationData;
static bool g_CalibrationInitialized = false;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static bool Calibration_LoadFromStorage(void);
static bool Calibration_SaveToStorage(void);
static void Calibration_SetDefaults(void);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool Calibration_Init(void)
{
    if (g_CalibrationInitialized)
    {
        return true;
    }

    /* Try to load calibration from storage */
    if (!Calibration_LoadFromStorage())
    {
        /* Set default calibration values */
        Calibration_SetDefaults();
        Calibration_SaveToStorage();
    }

    g_CalibrationInitialized = true;
    DEBUG_PRINT("Calibration initialized: Offset=%.2f, Gain=%.2f, Threshold=%.2f\r\n",
                g_CalibrationData.Offset, g_CalibrationData.Gain, g_CalibrationData.Threshold);

    return true;
}

bool Calibration_ProcessDownlink(const uint8_t *payload, uint8_t size,
                                  uint8_t *response, uint8_t *responseSize)
{
    if (payload == NULL || size < 2 || !g_CalibrationInitialized)
    {
        return false;
    }

    /* Parse command opcode */
    CalibrationCmd_t cmd = (CalibrationCmd_t)payload[0];
    bool success = false;

    switch (cmd)
    {
        case CALIB_CMD_RESET:
        {
            /* Reset to factory defaults */
            success = Calibration_Reset();
            DEBUG_PRINT("Calibration reset via downlink\r\n");
            break;
        }

        case CALIB_CMD_SET_OFFSET:
        {
            if (size >= 5)
            {
                /* Extract float offset value (4 bytes) */
                float offset;
                memcpy(&offset, &payload[1], sizeof(float));
                g_CalibrationData.Offset = offset;
                success = Calibration_SaveToStorage();
                DEBUG_PRINT("Calibration offset set to %.2f\r\n", offset);
            }
            break;
        }

        case CALIB_CMD_SET_GAIN:
        {
            if (size >= 5)
            {
                /* Extract float gain value (4 bytes) */
                float gain;
                memcpy(&gain, &payload[1], sizeof(float));
                g_CalibrationData.Gain = gain;
                success = Calibration_SaveToStorage();
                DEBUG_PRINT("Calibration gain set to %.2f\r\n", gain);
            }
            break;
        }

        case CALIB_CMD_SET_THRESHOLD:
        {
            if (size >= 5)
            {
                /* Extract float threshold value (4 bytes) */
                float threshold;
                memcpy(&threshold, &payload[1], sizeof(float));
                g_CalibrationData.Threshold = threshold;
                success = Calibration_SaveToStorage();
                DEBUG_PRINT("Calibration threshold set to %.2f\r\n", threshold);
            }
            break;
        }

        case CALIB_CMD_QUERY:
        {
            /* Return current calibration data */
            if (response != NULL && responseSize != NULL)
            {
                response[0] = CALIB_CMD_QUERY;
                memcpy(&response[1], &g_CalibrationData.Offset, sizeof(float));
                memcpy(&response[5], &g_CalibrationData.Gain, sizeof(float));
                memcpy(&response[9], &g_CalibrationData.Threshold, sizeof(float));
                *responseSize = 13;
                success = true;
                DEBUG_PRINT("Calibration query via downlink\r\n");
            }
            break;
        }

        default:
            DEBUG_PRINT("Unknown calibration command: 0x%02X\r\n", cmd);
            break;
    }

    /* Prepare acknowledgment response */
    if (response != NULL && responseSize != NULL && cmd != CALIB_CMD_QUERY)
    {
        response[0] = cmd;
        response[1] = success ? 0x01 : 0x00;  /* Success/Fail flag */
        *responseSize = 2;
    }

    return success;
}

bool Calibration_ProcessATCommand(const uint8_t *payload, uint8_t size)
{
    if (payload == NULL || size < 1 || !g_CalibrationInitialized)
    {
        return false;
    }

    /* Process AT calibration command (format similar to downlink) */
    uint8_t response[32];
    uint8_t responseSize = 0;

    return Calibration_ProcessDownlink(payload, size, response, &responseSize);
}

bool Calibration_GetData(CalibrationData_t *data)
{
    if (data == NULL || !g_CalibrationInitialized)
    {
        return false;
    }

    memcpy(data, &g_CalibrationData, sizeof(CalibrationData_t));
    return true;
}

bool Calibration_SetData(const CalibrationData_t *data)
{
    if (data == NULL || !g_CalibrationInitialized)
    {
        return false;
    }

    memcpy(&g_CalibrationData, data, sizeof(CalibrationData_t));
    return Calibration_SaveToStorage();
}

bool Calibration_Reset(void)
{
    if (!g_CalibrationInitialized)
    {
        return false;
    }

    Calibration_SetDefaults();
    return Calibration_SaveToStorage();
}

float Calibration_ApplyToValue(float rawValue)
{
    if (!g_CalibrationInitialized)
    {
        return rawValue;
    }

    /* Apply calibration: calibratedValue = (rawValue + offset) * gain */
    return (rawValue + g_CalibrationData.Offset) * g_CalibrationData.Gain;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static bool Calibration_LoadFromStorage(void)
{
    uint8_t buffer[sizeof(CalibrationData_t)];

    if (!Storage_Read(STORAGE_KEY_CALIBRATION, buffer, sizeof(buffer)))
    {
        return false;
    }

    /* Check if data is valid (simple validation) */
    CalibrationData_t *data = (CalibrationData_t*)buffer;
    if (data->Version == 0xFF)
    {
        return false;  /* Uninitialized flash */
    }

    memcpy(&g_CalibrationData, buffer, sizeof(CalibrationData_t));
    return true;
}

static bool Calibration_SaveToStorage(void)
{
    /* Update timestamp and version */
    g_CalibrationData.Timestamp = 0;  /* TODO: Use RTC timestamp */
    g_CalibrationData.Version = 1;

    return Storage_Write(STORAGE_KEY_CALIBRATION,
                        (const uint8_t*)&g_CalibrationData,
                        sizeof(CalibrationData_t));
}

static void Calibration_SetDefaults(void)
{
    /* Set default calibration values */
    g_CalibrationData.Offset = 0.0f;
    g_CalibrationData.Gain = 1.0f;
    g_CalibrationData.Threshold = 100.0f;
    g_CalibrationData.Timestamp = 0;
    g_CalibrationData.Version = 1;
}
