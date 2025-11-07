#include "sensor.h"
#include "config.h"
#include "hal_stubs.h"
#include "timer.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) \
    do                   \
    {                    \
    } while (0)
#endif

#ifndef SENSOR_FRAME_MAX_SIZE
#define SENSOR_FRAME_MAX_SIZE 64U
#endif

#ifndef SENSOR_FIFO_SIZE
#define SENSOR_FIFO_SIZE SENSOR_FRAME_MAX_SIZE
#endif

#ifndef SENSOR_WAKEUP_MIN_INTERVAL_S
#define SENSOR_WAKEUP_MIN_INTERVAL_S 5U
#endif

#ifndef SENSOR_WAKEUP_DEFAULT_MS
#define SENSOR_WAKEUP_DEFAULT_MS (SENSOR_WAKEUP_MIN_INTERVAL_S * 1000UL)
#endif

#ifndef SENSOR_HOUSEKEEPING_INTERVAL_MS
#define SENSOR_HOUSEKEEPING_INTERVAL_MS 15000UL
#endif

typedef struct
{
    uint8_t flagStartTimers;
    uint8_t flagActive;
    uint8_t lastCommand;
    uint8_t fifoReset;
    uint8_t fifoLength;
    uint8_t modeByte;
    uint8_t pendingCommand;
    uint8_t flagClear1;
    uint8_t flagClear2;
} SensorBridgeState_t;

typedef struct
{
    bool initialised;
    bool powered;
    SensorMode_t mode;
    SensorSample_t lastSample;
    SensorCalibrationState_t calibration;
    SensorBridgeState_t bridge;
    uint8_t fifoBuffer[SENSOR_FIFO_SIZE];
    uint16_t fifoSize;
    bool fifoValid;
    uint32_t wakeIntervalMs;
} SensorContext_t;

static SensorContext_t g_SensorCtx = {
    .initialised = false,
    .powered = false,
    .mode = SENSOR_MODE_LOW_POWER,
    .lastSample = { 0, 0, 0, false },
    .calibration = { 0U, 0U, 0U },
    .bridge = { 0U },
    .fifoBuffer = { 0U },
    .fifoSize = 0U,
    .fifoValid = false,
    .wakeIntervalMs = SENSOR_WAKEUP_DEFAULT_MS
};

static TimerEvent_t g_SensorWakeTimer;
static TimerEvent_t g_SensorHousekeepingTimer;
static bool g_SensorTimersReady = false;

/* ------------------------------------------------------------------------- */
/* Hardware hooks                                                            */
/* ------------------------------------------------------------------------- */
__attribute__((weak)) bool Sensor_BoardConfigure(void)
{
    return true;
}

__attribute__((weak)) bool Sensor_BoardPowerControl(bool enable)
{
    (void)enable;
    return true;
}

__attribute__((weak)) bool Sensor_BoardAcquire(uint16_t *primary, uint16_t *secondary)
{
    (void)primary;
    (void)secondary;
    return false;
}

__attribute__((weak)) bool Sensor_HwFetchFrame(uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen)
{
    (void)buffer;
    (void)maxLen;
    (void)actualLen;
    return false;
}

__attribute__((weak)) bool Sensor_HwPerformHandshake(void)
{
    return true;
}

__attribute__((weak)) void Sensor_HwFlushRx(void)
{
}

__attribute__((weak)) bool Sensor_HwReady(void)
{
    return true;
}

/* ------------------------------------------------------------------------- */
/* Local helpers                                                             */
/* ------------------------------------------------------------------------- */
static void Sensor_InitTimers(void);
static void Sensor_StopTimers(void);
static void Sensor_OnWakeTimer(void *context);
static void Sensor_OnHousekeepingTimer(void *context);
static void Sensor_ArmTimers(uint32_t delayMs);
static void Sensor_RequestCaptureCycle(void);
static void Sensor_PollInterface(void);
static void Sensor_HandleIncomingFrame(const uint8_t *data, uint16_t len);
static void Sensor_CopyIntoFifo(const uint8_t *data, uint16_t len);
static void Sensor_UpdateSampleFromBuffer(const uint8_t *buffer, uint16_t len);
static void Sensor_ProcessPending(void);
static void Sensor_SetWakeInterval(uint32_t seconds);
static uint32_t Sensor_ClampInterval(uint32_t seconds);
static bool Sensor_RunHandshake(void);
static void Sensor_LogFrame(const char *tag, const uint8_t *data, uint16_t len);
static void Sensor_FallbackSample(uint16_t *primary, uint16_t *secondary);
static uint16_t Sensor_ApplyCalibration(uint16_t raw, const SensorCalibrationState_t *calib);
static void Sensor_UpdateTimestamp(SensorSample_t *sample);

/* ------------------------------------------------------------------------- */
/* Core implementation                                                       */
/* ------------------------------------------------------------------------- */
bool Sensor_Init(void)
{
    if (g_SensorCtx.initialised)
    {
        return true;
    }

    if (!Sensor_BoardConfigure())
    {
        DEBUG_PRINT("Sensor: board configure failed\r\n");
        return false;
    }

    Sensor_ResetCalibration();
    Sensor_InitTimers();

    if (!Sensor_SetPower(true))
    {
        return false;
    }

    g_SensorCtx.mode = SENSOR_MODE_NORMAL;
    g_SensorCtx.bridge.modeByte = (uint8_t)g_SensorCtx.mode;
    g_SensorCtx.bridge.flagStartTimers = 1U;
    g_SensorCtx.initialised = true;
    DEBUG_PRINT("Sensor initialised\r\n");
    return true;
}

void Sensor_Deinit(void)
{
    if (!g_SensorCtx.initialised)
    {
        return;
    }

    Sensor_StopTimers();
    (void)Sensor_SetPower(false);
    g_SensorCtx.initialised = false;
    g_SensorCtx.lastSample.valid = false;
}

bool Sensor_IsInitialized(void)
{
    return g_SensorCtx.initialised;
}

bool Sensor_SetPower(bool enable)
{
    if (enable)
    {
        if (g_SensorCtx.powered)
        {
            return true;
        }

        if (!Sensor_BoardPowerControl(true))
        {
            return false;
        }

        if (SENSOR_POWER_ON_DELAY_MS > 0U)
        {
            HAL_Delay(SENSOR_POWER_ON_DELAY_MS);
        }

        g_SensorCtx.powered = true;
        g_SensorCtx.bridge.flagActive = 1U;
        g_SensorCtx.bridge.flagStartTimers = 1U;
        DEBUG_PRINT("Sensor power on\r\n");
        return true;
    }

    if (!g_SensorCtx.powered)
    {
        return true;
    }

    Sensor_StopTimers();
    (void)Sensor_BoardPowerControl(false);
    g_SensorCtx.powered = false;
    g_SensorCtx.bridge.flagActive = 0U;
    DEBUG_PRINT("Sensor power off\r\n");
    return true;
}

bool Sensor_IsPowered(void)
{
    return g_SensorCtx.powered;
}

bool Sensor_SetMode(SensorMode_t mode)
{
    if (mode < SENSOR_MODE_LOW_POWER || mode > SENSOR_MODE_HIGH_PRECISION)
    {
        return false;
    }

    g_SensorCtx.mode = mode;
    g_SensorCtx.bridge.modeByte = (uint8_t)mode;
    return true;
}

SensorMode_t Sensor_GetMode(void)
{
    return g_SensorCtx.mode;
}

bool Sensor_Read(SensorSample_t *sample)
{
    if (!g_SensorCtx.initialised || !g_SensorCtx.powered)
    {
        return false;
    }

    Sensor_Process();

    if (g_SensorCtx.lastSample.valid)
    {
        if (sample != NULL)
        {
            *sample = g_SensorCtx.lastSample;
        }
        return true;
    }

    uint16_t rawPrimary = 0;
    uint16_t rawSecondary = 0;

    if (!Sensor_BoardAcquire(&rawPrimary, &rawSecondary))
    {
        Sensor_FallbackSample(&rawPrimary, &rawSecondary);
    }

    rawPrimary = Sensor_ApplyCalibration(rawPrimary, &g_SensorCtx.calibration);
    rawSecondary = Sensor_ApplyCalibration(rawSecondary, &g_SensorCtx.calibration);

    g_SensorCtx.lastSample.primary = rawPrimary;
    g_SensorCtx.lastSample.secondary = rawSecondary;
    Sensor_UpdateTimestamp(&g_SensorCtx.lastSample);
    g_SensorCtx.lastSample.valid = true;

    if (sample != NULL)
    {
        *sample = g_SensorCtx.lastSample;
    }

    return true;
}

bool Sensor_GetLastSample(SensorSample_t *sample)
{
    if (!g_SensorCtx.lastSample.valid)
    {
        return false;
    }

    if (sample != NULL)
    {
        *sample = g_SensorCtx.lastSample;
    }
    return true;
}

bool Sensor_GetFrame(uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen)
{
    if (!g_SensorCtx.initialised || buffer == NULL || maxLen == 0U)
    {
        return false;
    }

    if (!g_SensorCtx.fifoValid)
    {
        Sensor_Process();
    }

    if (!g_SensorCtx.fifoValid)
    {
        return false;
    }

    uint16_t copyLen = (g_SensorCtx.fifoSize > maxLen) ? maxLen : g_SensorCtx.fifoSize;
    memcpy(buffer, g_SensorCtx.fifoBuffer, copyLen);

    if (actualLen != NULL)
    {
        *actualLen = copyLen;
    }

    return true;
}

void Sensor_ResetCalibration(void)
{
    g_SensorCtx.calibration.parameter = 0U;
    g_SensorCtx.calibration.value = 0U;
    g_SensorCtx.calibration.applyCount = 0U;
}

bool Sensor_UpdateCalibration(uint32_t parameter, uint32_t value)
{
    g_SensorCtx.calibration.parameter = parameter;
    g_SensorCtx.calibration.value = value;
    g_SensorCtx.calibration.applyCount++;

    DEBUG_PRINT("Sensor calibration update param=%lu value=%lu\r\n",
                (unsigned long)parameter, (unsigned long)value);
    return true;
}

const SensorCalibrationState_t *Sensor_GetCalibration(void)
{
    return &g_SensorCtx.calibration;
}

void Sensor_Process(void)
{
    if (!g_SensorCtx.initialised)
    {
        return;
    }

    Sensor_PollInterface();
    Sensor_ProcessPending();
}

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */
static void Sensor_InitTimers(void)
{
    if (g_SensorTimersReady)
    {
        return;
    }

    TimerInit(&g_SensorWakeTimer, Sensor_OnWakeTimer);
    TimerSetContext(&g_SensorWakeTimer, &g_SensorCtx);
    TimerInit(&g_SensorHousekeepingTimer, Sensor_OnHousekeepingTimer);
    TimerSetContext(&g_SensorHousekeepingTimer, &g_SensorCtx);
    g_SensorTimersReady = true;
}

static void Sensor_StopTimers(void)
{
    if (!g_SensorTimersReady)
    {
        return;
    }

    TimerStop(&g_SensorWakeTimer);
    TimerStop(&g_SensorHousekeepingTimer);
}

static void Sensor_OnWakeTimer(void *context)
{
    SensorContext_t *ctx = (SensorContext_t *)context;
    if (ctx == NULL)
    {
        return;
    }

    ctx->bridge.flagStartTimers = 1U;
}

static void Sensor_OnHousekeepingTimer(void *context)
{
    SensorContext_t *ctx = (SensorContext_t *)context;
    if (ctx == NULL)
    {
        return;
    }

    ctx->bridge.flagClear1 = 0U;
    ctx->bridge.flagClear2 = 0U;
    ctx->bridge.pendingCommand = 1U;

    if (!TimerIsStarted(&g_SensorWakeTimer))
    {
        ctx->bridge.flagStartTimers = 1U;
    }
}

static void Sensor_ArmTimers(uint32_t delayMs)
{
    if (!g_SensorTimersReady)
    {
        Sensor_InitTimers();
    }

    if (delayMs == 0U)
    {
        delayMs = SENSOR_WAKEUP_DEFAULT_MS;
    }

    TimerStop(&g_SensorWakeTimer);
    TimerSetValue(&g_SensorWakeTimer, delayMs);
    TimerStart(&g_SensorWakeTimer);

    TimerStop(&g_SensorHousekeepingTimer);
    TimerSetValue(&g_SensorHousekeepingTimer, SENSOR_HOUSEKEEPING_INTERVAL_MS);
    TimerStart(&g_SensorHousekeepingTimer);
}

static void Sensor_RequestCaptureCycle(void)
{
    g_SensorCtx.bridge.flagStartTimers = 0U;

    if (!g_SensorCtx.powered)
    {
        g_SensorCtx.bridge.pendingCommand = 1U;
        return;
    }

    if (!Sensor_RunHandshake())
    {
        g_SensorCtx.bridge.pendingCommand = 1U;
        return;
    }

    g_SensorCtx.bridge.pendingCommand = 0U;
    Sensor_HwFlushRx();
    Sensor_ArmTimers(g_SensorCtx.wakeIntervalMs);
}

static void Sensor_PollInterface(void)
{
    uint8_t buffer[SENSOR_FRAME_MAX_SIZE] = { 0 };
    uint16_t length = 0;

    while (Sensor_HwFetchFrame(buffer, sizeof(buffer), &length))
    {
        if (length == 0U)
        {
            break;
        }

        if (length > SENSOR_FRAME_MAX_SIZE)
        {
            length = SENSOR_FRAME_MAX_SIZE;
        }

        Sensor_HandleIncomingFrame(buffer, length);
        length = 0U;
    }
}

static void Sensor_HandleIncomingFrame(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    g_SensorCtx.bridge.lastCommand = data[0];
    g_SensorCtx.bridge.fifoLength = (uint8_t)len;

    if (len == 4U)
    {
        uint32_t seconds = (uint32_t)data[1] |
                           ((uint32_t)data[2] << 8U) |
                           ((uint32_t)data[3] << 16U);
        Sensor_SetWakeInterval(seconds);
    }

    if (g_SensorCtx.mode != SENSOR_MODE_LOW_POWER)
    {
        Sensor_CopyIntoFifo(data, len);
    }

    Sensor_UpdateSampleFromBuffer(data, len);

    if (DEBUG_ENABLED && g_SensorCtx.bridge.flagActive)
    {
        Sensor_LogFrame("RX", data, len);
    }
}

static void Sensor_CopyIntoFifo(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    uint16_t bounded = (len > SENSOR_FIFO_SIZE) ? SENSOR_FIFO_SIZE : len;
    memcpy(g_SensorCtx.fifoBuffer, data, bounded);
    g_SensorCtx.fifoSize = bounded;
    g_SensorCtx.fifoValid = true;
    g_SensorCtx.bridge.fifoReset = 0U;
}

static void Sensor_UpdateSampleFromBuffer(const uint8_t *buffer, uint16_t len)
{
    if (buffer == NULL || len < 4U)
    {
        return;
    }

    uint16_t primary = (uint16_t)((uint16_t)buffer[0] << 8U) | buffer[1];
    uint16_t secondary = (uint16_t)((uint16_t)buffer[2] << 8U) | buffer[3];

    primary = Sensor_ApplyCalibration(primary, &g_SensorCtx.calibration);
    secondary = Sensor_ApplyCalibration(secondary, &g_SensorCtx.calibration);

    g_SensorCtx.lastSample.primary = primary;
    g_SensorCtx.lastSample.secondary = secondary;
    Sensor_UpdateTimestamp(&g_SensorCtx.lastSample);
    g_SensorCtx.lastSample.valid = true;
}

static void Sensor_ProcessPending(void)
{
    if (g_SensorCtx.bridge.flagStartTimers)
    {
        Sensor_RequestCaptureCycle();
    }
}

static void Sensor_SetWakeInterval(uint32_t seconds)
{
    uint32_t clamped = Sensor_ClampInterval(seconds);
    g_SensorCtx.wakeIntervalMs = clamped * 1000UL;
    g_SensorCtx.bridge.flagStartTimers = 1U;
    g_SensorCtx.bridge.flagActive = 1U;
}

static uint32_t Sensor_ClampInterval(uint32_t seconds)
{
    if (seconds < SENSOR_WAKEUP_MIN_INTERVAL_S)
    {
        seconds = SENSOR_WAKEUP_MIN_INTERVAL_S;
    }

    uint32_t maxSeconds = UINT32_MAX / 1000UL;
    if (seconds > maxSeconds)
    {
        seconds = maxSeconds;
    }

    return seconds;
}

static bool Sensor_RunHandshake(void)
{
    if (!Sensor_HwReady())
    {
        return false;
    }

    if (!Sensor_HwPerformHandshake())
    {
        DEBUG_PRINT("Sensor: handshake failed\r\n");
        return false;
    }

    if (DEBUG_ENABLED)
    {
        DEBUG_PRINT("Sensor: handshake complete\r\n");
    }
    return true;
}

static void Sensor_LogFrame(const char *tag, const uint8_t *data, uint16_t len)
{
#if DEBUG_ENABLED
    if ((tag == NULL) || (data == NULL))
    {
        return;
    }

    DEBUG_PRINT("[SENSOR][%s] ", tag);
    for (uint16_t i = 0; i < len; i++)
    {
        DEBUG_PRINT("%02X", data[i]);
    }
    DEBUG_PRINT("\r\n");
#else
    (void)tag;
    (void)data;
    (void)len;
#endif
}

static void Sensor_FallbackSample(uint16_t *primary, uint16_t *secondary)
{
    if ((primary == NULL) || (secondary == NULL))
    {
        return;
    }

    uint32_t tick = HAL_GetTick();
    *primary = (uint16_t)((tick / 8U) & 0x0FFFU);
    *secondary = (uint16_t)((tick / 5U) & 0x0FFFU);
}

static uint16_t Sensor_ApplyCalibration(uint16_t raw, const SensorCalibrationState_t *calib)
{
    uint16_t offset = (uint16_t)(calib->parameter & 0x0FFFU);
    uint16_t gain = (uint16_t)(((calib->value >> 8) & 0x00FFU) + 1U);

    uint32_t result = (uint32_t)raw + offset;
    result = (result * gain) / 4U;
    if (result > 0xFFFFU)
    {
        result = 0xFFFFU;
    }
    return (uint16_t)result;
}

static void Sensor_UpdateTimestamp(SensorSample_t *sample)
{
    if (sample == NULL)
    {
        return;
    }
    sample->timestampMs = HAL_GetTick();
}
