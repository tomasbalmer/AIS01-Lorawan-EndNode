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
