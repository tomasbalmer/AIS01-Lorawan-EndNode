#include "uplink_encoder.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static bool UplinkEncoder_CheckBuffer(UplinkPayload_t *out, uint8_t requiredSize)
{
    if ((out == NULL) || (out->buffer == NULL) || (out->maxSize < requiredSize))
    {
        return false;
    }
    out->size = 0U;
    return true;
}

bool UplinkEncoder_EncodeStatus(const UplinkStatusContext_t *ctx, UplinkPayload_t *out)
{
    const uint8_t requiredSize = 12U;

    if ((ctx == NULL) || !UplinkEncoder_CheckBuffer(out, requiredSize))
    {
        return false;
    }

    uint8_t *buffer = out->buffer;
    buffer[0] = 0x01U;
    buffer[1] = ctx->batteryLevel;
    buffer[2] = ctx->adrEnabled;
    buffer[3] = ctx->dataRate;
    buffer[4] = ctx->txPower;
    buffer[5] = ctx->freqBand;
    buffer[6] = (uint8_t)ctx->rssi;
    buffer[7] = (uint8_t)ctx->snr;
    buffer[8] = (uint8_t)(ctx->frameCounterUp & 0xFFU);
    buffer[9] = (uint8_t)((ctx->frameCounterUp >> 8) & 0xFFU);
    buffer[10] = (uint8_t)((ctx->frameCounterUp >> 16) & 0xFFU);
    buffer[11] = (uint8_t)((ctx->frameCounterUp >> 24) & 0xFFU);

    out->size = requiredSize;
    return true;
}

bool UplinkEncoder_EncodeCalibration(const uint8_t *calData, uint8_t calSize, UplinkPayload_t *out)
{
    uint8_t requiredSize;

    if ((calData == NULL) || (calSize == 0U))
    {
        return false;
    }

    requiredSize = (uint8_t)(calSize + 1U);

    if (!UplinkEncoder_CheckBuffer(out, requiredSize))
    {
        return false;
    }

    out->buffer[0] = 0xA0U;
    memcpy(&out->buffer[1], calData, calSize);
    out->size = requiredSize;
    return true;
}

bool UplinkEncoder_EncodeDebug(uint8_t fwMajor,
                               uint8_t fwMinor,
                               uint8_t fwPatch,
                               uint8_t loraState,
                               UplinkPayload_t *out)
{
    const uint8_t requiredSize = 6U;

    if (!UplinkEncoder_CheckBuffer(out, requiredSize))
    {
        return false;
    }

    uint8_t *b = out->buffer;
    b[0] = 0xF0U;
    b[1] = fwMajor;
    b[2] = fwMinor;
    b[3] = fwPatch;
    b[4] = loraState;
    b[5] = 0x00U;

    out->size = requiredSize;
    return true;
}

bool UplinkEncoder_EncodeSensorStub(UplinkPayload_t *out)
{
#ifndef DEBUG_STUB
    /* Stub disabled for production */
    return false;
#else
    const uint8_t requiredSize = 2U;

    if (!UplinkEncoder_CheckBuffer(out, requiredSize))
    {
        return false;
    }

    /* Stub: indicates "sensor frame not implemented" */
    out->buffer[0] = 0x02U;
    out->buffer[1] = 0xFFU;

    out->size = requiredSize;
    return true;
#endif
}

bool UplinkEncoder_EncodeSensorFrame(const SensorSample_t *sample,
                                     uint8_t batteryLevel,
                                     UplinkPayload_t *out)
{
    const uint8_t requiredSize = 12U;

    if ((sample == NULL) ||
        (sample->valid == false) ||
        !UplinkEncoder_CheckBuffer(out, requiredSize))
    {
        return false;
    }

    uint8_t *b = out->buffer;

    b[0] = 0x02U;
    b[1] = batteryLevel;
    b[2] = 0x00U;
    b[3] = 0x00U;

    b[4] = (uint8_t)(sample->primary & 0xFFU);
    b[5] = (uint8_t)((sample->primary >> 8) & 0xFFU);

    b[6] = (uint8_t)(sample->secondary & 0xFFU);
    b[7] = (uint8_t)((sample->secondary >> 8) & 0xFFU);

    uint32_t ts = sample->timestampMs;
    b[8] = (uint8_t)(ts & 0xFFU);
    b[9] = (uint8_t)((ts >> 8) & 0xFFU);
    b[10] = (uint8_t)((ts >> 16) & 0xFFU);
    b[11] = (uint8_t)((ts >> 24) & 0xFFU);

    out->size = requiredSize;
    return true;
}

bool UplinkEncoder_EncodeSensorStats(const UplinkSensorStatsContext_t *ctx,
                                     UplinkPayload_t *out)
{
    const uint8_t required = 21U;
    if ((ctx == NULL) || !UplinkEncoder_CheckBuffer(out, required))
    {
        return false;
    }

    uint8_t *b = out->buffer;
    b[0] = 0x03U;
    b[1] = ctx->batteryLevel;
    b[2] = ctx->motionState;
    b[3] = ctx->occupancyState;
    b[4] = ctx->sensorMode;

    b[5] = (uint8_t)(ctx->primaryMin & 0xFFU);
    b[6] = (uint8_t)((ctx->primaryMin >> 8) & 0xFFU);
    b[7] = (uint8_t)(ctx->primaryAvg & 0xFFU);
    b[8] = (uint8_t)((ctx->primaryAvg >> 8) & 0xFFU);
    b[9] = (uint8_t)(ctx->primaryMax & 0xFFU);
    b[10] = (uint8_t)((ctx->primaryMax >> 8) & 0xFFU);

    b[11] = (uint8_t)(ctx->secondaryMin & 0xFFU);
    b[12] = (uint8_t)((ctx->secondaryMin >> 8) & 0xFFU);
    b[13] = (uint8_t)(ctx->secondaryAvg & 0xFFU);
    b[14] = (uint8_t)((ctx->secondaryAvg >> 8) & 0xFFU);
    b[15] = (uint8_t)(ctx->secondaryMax & 0xFFU);
    b[16] = (uint8_t)((ctx->secondaryMax >> 8) & 0xFFU);

    uint32_t ts = ctx->timestampMs;
    b[17] = (uint8_t)(ts & 0xFFU);
    b[18] = (uint8_t)((ts >> 8) & 0xFFU);
    b[19] = (uint8_t)((ts >> 16) & 0xFFU);
    b[20] = (uint8_t)((ts >> 24) & 0xFFU);

    out->size = required;
    return true;
}

bool UplinkEncoder_EncodeStatusEx(const UplinkStatusExContext_t *ctx,
                                  UplinkPayload_t *out)
{
    const uint8_t required = 21U;
    if ((ctx == NULL) || !UplinkEncoder_CheckBuffer(out, required))
    {
        return false;
    }

    uint8_t *b = out->buffer;

    b[0] = 0xF1U;
    b[1] = ctx->batteryLevel;
    b[2] = ctx->adrEnabled;
    b[3] = ctx->dataRate;
    b[4] = ctx->txPower;
    b[5] = ctx->freqBand;

    b[6] = (uint8_t)(ctx->batteryMv & 0xFFU);
    b[7] = (uint8_t)((ctx->batteryMv >> 8) & 0xFFU);

    b[8] = ctx->sensorPowered;
    b[9] = ctx->sensorMode;
    b[10] = ctx->pendingDl;

    b[11] = (uint8_t)ctx->rssi;
    b[12] = (uint8_t)ctx->snr;

    uint32_t up = ctx->uptimeSec;
    b[13] = (uint8_t)(up & 0xFFU);
    b[14] = (uint8_t)((up >> 8) & 0xFFU);
    b[15] = (uint8_t)((up >> 16) & 0xFFU);
    b[16] = (uint8_t)((up >> 24) & 0xFFU);

    uint32_t fcnt = ctx->frameCounterUp;
    b[17] = (uint8_t)(fcnt & 0xFFU);
    b[18] = (uint8_t)((fcnt >> 8) & 0xFFU);
    b[19] = (uint8_t)((fcnt >> 16) & 0xFFU);
    b[20] = (uint8_t)((fcnt >> 24) & 0xFFU);

    out->size = required;
    return true;
}

bool UplinkEncoder_EncodeMacMirror(const UplinkMacMirrorContext_t *ctx,
                                   UplinkPayload_t *out)
{
    if ((ctx == NULL) || (ctx->size == 0U) || (ctx->size > MAC_MIRROR_MAX_SIZE))
    {
        return false;
    }

    uint8_t required = (uint8_t)(1U + ctx->size);

    if (!UplinkEncoder_CheckBuffer(out, required))
    {
        return false;
    }

    out->buffer[0] = 0xF2U;
    memcpy(&out->buffer[1], ctx->payload, ctx->size);
    out->size = required;

    return true;
}
