/*!
 * \file      calibration.c
 *
 * \brief     Remote calibration / configuration engine shim
 *
 * \details   Mirrors the behaviour observed in the stock firmware: payloads are
 *            buffered in RAM, validated against the previous mirror and then
 *            applied to hardware-facing bitfields before signalling completion.
 *            Persistence is *not* performed here; callers may add it on top.
 */
#include <string.h>
#include "calibration.h"
#include "config.h"
#include "sensor.h"
#include <stdio.h>

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) do { } while (0)
#endif

/* ============================================================================
 * LOCAL CONSTANTS
 * ========================================================================== */
#define CALIB_FLAGS_PENDING_MASK      (0x0FU)
#define CALIB_FLAGS_APPLY_SLOT_MASK   (0xF0U)

/* ============================================================================
 * LOCAL TYPES
 * ========================================================================== */
typedef struct
{
    uint8_t Active[CALIBRATION_BUFFER_SIZE];
    uint8_t Mirror[CALIBRATION_BUFFER_SIZE];
    uint8_t Length;
} CalibrationBuffer_t;

typedef struct
{
    CalibrationBuffer_t Buffer;
    CalibrationData_t Snapshot;
} CalibrationState_t;

/* ============================================================================
 * LOCAL STORAGE
 * ========================================================================== */
static CalibrationState_t g_CalibrationState;
static bool g_CalibrationInitialized = false;

/* ============================================================================
 * WEAK HARDWARE HOOKS
 * ========================================================================== */
__attribute__((weak)) bool Calibration_HwWaitReady(void)
{
    return true;
}

__attribute__((weak)) void Calibration_HwSetEnable(bool enable)
{
    (void)enable;
}

/* ============================================================================
 * LOCAL HELPERS
 * ========================================================================== */
static void Calibration_ClearState(void)
{
    memset(&g_CalibrationState, 0, sizeof(g_CalibrationState));
}

static uint32_t Calibration_ReadU32(const uint8_t *buffer, uint8_t offset, uint8_t length)
{
    uint32_t value = 0;
    if (length >= (offset + sizeof(uint32_t)))
    {
        memcpy(&value, &buffer[offset], sizeof(uint32_t));
    }
    return value;
}

static void Calibration_ParseSnapshot(void)
{
    CalibrationBuffer_t *buf = &g_CalibrationState.Buffer;
    CalibrationData_t *snap = &g_CalibrationState.Snapshot;

    memset(snap, 0, sizeof(*snap));

    if (buf->Length > 0)
    {
        snap->Command = buf->Active[0];
    }
    if (buf->Length > 1)
    {
        snap->Flags = buf->Active[1];
    }

    snap->Parameter = Calibration_ReadU32(buf->Active, 0x04U, buf->Length);
    snap->Value     = Calibration_ReadU32(buf->Active, 0x10U, buf->Length);
    snap->ApplyFlag = Calibration_ReadU32(buf->Active, 0x18U, buf->Length);

    snap->PendingSlots[0] = snap->Flags & CALIB_FLAGS_PENDING_MASK;
    snap->PendingSlots[1] = (snap->Flags & CALIB_FLAGS_APPLY_SLOT_MASK) >> 4;
    snap->PendingSlots[2] = (uint8_t)((snap->ApplyFlag & 0xF) & 0xFFU);
    snap->Busy = (snap->ApplyFlag != 0U);
}

static bool Calibration_CopyPayload(const uint8_t *payload, uint8_t size)
{
    CalibrationBuffer_t *buf = &g_CalibrationState.Buffer;

    if (size == 0U || size > CALIBRATION_BUFFER_SIZE)
    {
        return false;
    }

    memcpy(buf->Active, payload, size);
    buf->Length = size;
    return true;
}

static void Calibration_UpdateMirror(void)
{
    CalibrationBuffer_t *buf = &g_CalibrationState.Buffer;
    memcpy(buf->Mirror, buf->Active, buf->Length);
}

static bool Calibration_ApplyToHardware(void)
{
    CalibrationData_t *snap = &g_CalibrationState.Snapshot;

    snap->Busy = true;
    if (!Calibration_HwWaitReady())
    {
        snap->Busy = false;
        return false;
    }

    Calibration_HwSetEnable(true);
    Calibration_HwSetEnable(false);

    snap->ApplyFlag = 0U;
    snap->Busy = false;
    return true;
}

static bool Calibration_HandleApply(void)
{
    if (!Calibration_ApplyToHardware())
    {
        return false;
    }

    Calibration_UpdateMirror();
    (void)Sensor_UpdateCalibration(g_CalibrationState.Snapshot.Parameter,
                                   g_CalibrationState.Snapshot.Value);
    g_CalibrationState.Snapshot.PendingSlots[0] = 0U;
    g_CalibrationState.Snapshot.PendingSlots[1] = 0U;
    g_CalibrationState.Snapshot.PendingSlots[2] = 0U;
    DEBUG_PRINT("Calibration apply complete (param=%lu, value=%lu)\r\n",
                (unsigned long)g_CalibrationState.Snapshot.Parameter,
                (unsigned long)g_CalibrationState.Snapshot.Value);
    return true;
}

static bool Calibration_HandleQuery(uint8_t *response, uint8_t *responseSize)
{
    if (response == NULL || responseSize == NULL)
    {
        return true; /* Nothing to serialise */
    }

    CalibrationBuffer_t *buf = &g_CalibrationState.Buffer;

    if (buf->Length == 0U)
    {
        response[0] = CALIB_OPCODE_QUERY;
        response[1] = 0U;
        *responseSize = 2U;
        return true;
    }

    memcpy(response, buf->Mirror, buf->Length);
    *responseSize = buf->Length;
    return true;
}

/* ============================================================================
 * PUBLIC API
 * ========================================================================== */

bool Calibration_Init(void)
{
    if (g_CalibrationInitialized)
    {
        return true;
    }

    Calibration_ClearState();
    g_CalibrationInitialized = true;
    DEBUG_PRINT("Calibration context initialised\r\n");
    return true;
}

bool Calibration_ProcessDownlink(const uint8_t *payload, uint8_t size,
                                  uint8_t *response, uint8_t *responseSize)
{
    if (!g_CalibrationInitialized || payload == NULL || size == 0U)
    {
        return false;
    }

    const uint8_t opcode = payload[0];

    if (opcode == CALIB_OPCODE_QUERY)
    {
        return Calibration_HandleQuery(response, responseSize);
    }
    else if (opcode == CALIB_OPCODE_APPLY)
    {
        if (!Calibration_CopyPayload(payload, size))
        {
            return false;
        }

        Calibration_ParseSnapshot();

        if (!Calibration_HandleApply())
        {
            return false;
        }

        if (response != NULL && responseSize != NULL)
        {
            response[0] = CALIB_OPCODE_APPLY;
            response[1] = 0x01U; /* success */
            *responseSize = 2U;
        }
        return true;
    }

    return false;
}

bool Calibration_GetData(CalibrationData_t *data)
{
    if (!g_CalibrationInitialized || data == NULL)
    {
        return false;
    }

    *data = g_CalibrationState.Snapshot;
    return true;
}

bool Calibration_Reset(void)
{
    if (!g_CalibrationInitialized)
    {
        return false;
    }

    Calibration_ClearState();
    return true;
}

bool Calibration_IsBusy(void)
{
    return g_CalibrationInitialized && g_CalibrationState.Snapshot.Busy;
}

bool Calibration_HasPending(void)
{
    if (!g_CalibrationInitialized)
    {
        return false;
    }

    const CalibrationData_t *snap = &g_CalibrationState.Snapshot;
    return ((snap->PendingSlots[0] | snap->PendingSlots[1] |
             snap->PendingSlots[2] | (uint8_t)(snap->ApplyFlag & 0xFFU)) != 0U);
}
