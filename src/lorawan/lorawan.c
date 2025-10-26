#include "lorawan.h"
#include "lorawan_crypto.h"
#include "lorawan_region.h"
#include "radio/sx1276/sx1276.h"
#include "board/board.h"
#include "storage.h"
#include "config.h"
#include <string.h>

#define UPSTREAM_DIR   0
#define DOWNSTREAM_DIR 1

static LoRaWANStatus_t LoRaWAN_BuildJoinRequest(LoRaWANContext_t *ctx, uint8_t *buffer, uint8_t *size);
static LoRaWANStatus_t LoRaWAN_ParseJoinAccept(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size);
static LoRaWANStatus_t LoRaWAN_BuildUplink(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size, uint8_t port, LoRaWANMsgType_t msgType, uint8_t *out, uint8_t *outLen);

extern SX1276_t SX1276;

LoRaWANStatus_t LoRaWAN_Init(LoRaWANContext_t *ctx)
{
    if (ctx == NULL || ctx->Session == NULL || ctx->RadioBuffer == NULL || ctx->RadioBufferSize < 255)
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    ctx->Session->Joined = false;

    SX1276SetPublicNetwork( true );

    return LORAWAN_STATUS_SUCCESS;
}

LoRaWANStatus_t LoRaWAN_RequestJoin(LoRaWANContext_t *ctx)
{
    if (ctx == NULL)
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }
    uint8_t frame[32];
    uint8_t frameLen = 0;
    LoRaWANStatus_t status = LoRaWAN_BuildJoinRequest(ctx, frame, &frameLen);
    if (status != LORAWAN_STATUS_SUCCESS)
    {
        return status;
    }

    SX1276SetChannel( LoRaWAN_RegionGetJoinFrequency(ctx->Settings.Region, ctx->Session->DevNonceCounter) );
    SX1276SetTxConfig( MODEM_LORA, ctx->Settings.TxPower, 0, 0, ctx->Settings.DataRate, 1,
                       0, false, true, 0, 0, false, 4000 );

    SX1276Send( frame, frameLen );
    return LORAWAN_STATUS_SUCCESS;
}

LoRaWANStatus_t LoRaWAN_Send(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size, uint8_t port, LoRaWANMsgType_t msgType)
{
    if (ctx == NULL || !ctx->Session->Joined)
    {
        return LORAWAN_STATUS_NOT_JOINED;
    }

    uint8_t frame[255];
    uint8_t frameLen = 0;
    LoRaWANStatus_t status = LoRaWAN_BuildUplink(ctx, buffer, size, port, msgType, frame, &frameLen);
    if (status != LORAWAN_STATUS_SUCCESS)
    {
        return status;
    }

    uint8_t channel = LoRaWAN_RegionGetNextChannel(ctx->Settings.Region, NULL);
    SX1276SetChannel( LoRaWAN_RegionGetUplinkFrequency(ctx->Settings.Region, channel) );
    SX1276SetTxConfig( MODEM_LORA, ctx->Settings.TxPower, 0, 0, ctx->Settings.DataRate, 1,
                       0, false, true, 0, 0, false, 3000 );
    SX1276Send( frame, frameLen );

    ctx->Session->FCntUp++;
    Storage_UpdateFrameCounters(ctx->Session->FCntUp, ctx->Session->FCntDown);

    return LORAWAN_STATUS_SUCCESS;
}

void LoRaWAN_Process(LoRaWANContext_t *ctx)
{
    (void)ctx;
}

void LoRaWAN_RunRxWindow(LoRaWANContext_t *ctx, uint8_t window)
{
    (void)ctx;
    (void)window;
}

void LoRaWAN_HandleRadioEvent(LoRaWANContext_t *ctx)
{
    (void)ctx;
}

static LoRaWANStatus_t LoRaWAN_BuildJoinRequest(LoRaWANContext_t *ctx, uint8_t *buffer, uint8_t *size)
{
    if (ctx == NULL || buffer == NULL || size == NULL)
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    uint8_t idx = 0;
    buffer[idx++] = 0x00; /* Join-request MHDR */
    memcpy(&buffer[idx], ctx->Session->AppEui, 8);
    idx += 8;
    memcpy(&buffer[idx], ctx->Session->DevEui, 8);
    idx += 8;

    uint16_t devNonce = (uint16_t)(ctx->Session->DevNonceCounter & 0xFFFF);
    buffer[idx++] = devNonce & 0xFF;
    buffer[idx++] = (devNonce >> 8) & 0xFF;

    uint32_t mic = 0;
    if (!LoRaWAN_Crypto_ComputeJoinMic(ctx->Session->AppKey, buffer, idx, &mic))
    {
        return LORAWAN_STATUS_ERROR;
    }
    buffer[idx++] = mic & 0xFF;
    buffer[idx++] = (mic >> 8) & 0xFF;
    buffer[idx++] = (mic >> 16) & 0xFF;
    buffer[idx++] = (mic >> 24) & 0xFF;

    *size = idx;
    ctx->Session->DevNonceCounter++;
    return LORAWAN_STATUS_SUCCESS;
}

static LoRaWANStatus_t LoRaWAN_ParseJoinAccept(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size)
{
    if (ctx == NULL || buffer == NULL || size < 17)
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint8_t decrypted[32];
    if (!LoRaWAN_Crypto_EncryptPayload(ctx->Session->AppKey, buffer, size, 0, 0, DOWNSTREAM_DIR, decrypted))
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint32_t mic = 0;
    if (!LoRaWAN_Crypto_ComputeJoinMic(ctx->Session->AppKey, decrypted, size - 4, &mic))
    {
        return LORAWAN_STATUS_ERROR;
    }
    uint32_t rxMic = decrypted[size - 4] | (decrypted[size - 3] << 8) | (decrypted[size - 2] << 16) | (decrypted[size - 1] << 24);
    if (mic != rxMic)
    {
        return LORAWAN_STATUS_ERROR;
    }

    const uint8_t *appNonce = &decrypted[1];
    const uint8_t *netId = &decrypted[4];
    ctx->Session->DevAddr = decrypted[7] | (decrypted[8] << 8) | (decrypted[9] << 16) | (decrypted[10] << 24);

    uint16_t devNonce = (uint16_t)((ctx->Session->DevNonceCounter - 1) & 0xFFFF);
    uint8_t devNonceBytes[2] = { devNonce & 0xFF, (devNonce >> 8) & 0xFF };

    if (!LoRaWAN_Crypto_ComputeJoinKeys(ctx->Session->AppKey, appNonce, netId, devNonceBytes, ctx->Session->NwkSKey, ctx->Session->AppSKey))
    {
        return LORAWAN_STATUS_ERROR;
    }

    ctx->Session->Joined = true;
    ctx->Session->FCntUp = 0;
    ctx->Session->FCntDown = 0;

    if (ctx->Callbacks.OnJoinSuccess)
    {
        ctx->Callbacks.OnJoinSuccess(ctx->Session->DevAddr);
    }

    Storage_UpdateJoinKeys(ctx->Session->DevAddr, ctx->Session->NwkSKey, ctx->Session->AppSKey);

    return LORAWAN_STATUS_SUCCESS;
}

static LoRaWANStatus_t LoRaWAN_BuildUplink(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size, uint8_t port, LoRaWANMsgType_t msgType, uint8_t *out, uint8_t *outLen)
{
    if (ctx == NULL || buffer == NULL || out == NULL || outLen == NULL)
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    uint8_t idx = 0;
    out[idx++] = (msgType == LORAWAN_MSG_CONFIRMED) ? 0x80 : 0x40;
    out[idx++] = ctx->Session->DevAddr & 0xFF;
    out[idx++] = (ctx->Session->DevAddr >> 8) & 0xFF;
    out[idx++] = (ctx->Session->DevAddr >> 16) & 0xFF;
    out[idx++] = (ctx->Session->DevAddr >> 24) & 0xFF;

    out[idx++] = 0x00; /* FCtrl */
    out[idx++] = ctx->Session->FCntUp & 0xFF;
    out[idx++] = (ctx->Session->FCntUp >> 8) & 0xFF;

    out[idx++] = port;

    uint8_t encPayload[255];
    if (!LoRaWAN_Crypto_EncryptPayload(ctx->Session->AppSKey, buffer, size, ctx->Session->DevAddr, ctx->Session->FCntUp, UPSTREAM_DIR, encPayload))
    {
        return LORAWAN_STATUS_ERROR;
    }
    memcpy(&out[idx], encPayload, size);
    idx += size;

    uint32_t mic = 0;
    if (!LoRaWAN_Crypto_ComputeMic(ctx->Session->NwkSKey, out, idx, ctx->Session->DevAddr, ctx->Session->FCntUp, UPSTREAM_DIR, &mic))
    {
        return LORAWAN_STATUS_ERROR;
    }

    out[idx++] = mic & 0xFF;
    out[idx++] = (mic >> 8) & 0xFF;
    out[idx++] = (mic >> 16) & 0xFF;
    out[idx++] = (mic >> 24) & 0xFF;

    *outLen = idx;
    return LORAWAN_STATUS_SUCCESS;
}
