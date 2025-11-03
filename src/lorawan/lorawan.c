#include "lorawan.h"
#include "lorawan_crypto.h"
#include "lorawan_region.h"
#include "radio.h"
#include "board/board.h"
#include "storage.h"
#include "config.h"
#include "timer.h"
#include <string.h>

#define UPSTREAM_DIR   0
#define DOWNSTREAM_DIR 1

static LoRaWANStatus_t LoRaWAN_BuildJoinRequest(LoRaWANContext_t *ctx, uint8_t *buffer, uint8_t *size);
static LoRaWANStatus_t LoRaWAN_ParseJoinAccept(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size);
static LoRaWANStatus_t LoRaWAN_BuildUplink(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size, uint8_t port, LoRaWANMsgType_t msgType, uint8_t *out, uint8_t *outLen);

typedef enum
{
    LORAWAN_OP_NONE = 0,
    LORAWAN_OP_JOIN,
    LORAWAN_OP_TX
} LoRaWANOperation_t;

static LoRaWANContext_t *g_ActiveCtx = NULL;
static RadioEvents_t g_RadioEvents;
static TimerEvent_t g_Rx1Timer;
static TimerEvent_t g_Rx2Timer;
static LoRaWANOperation_t g_CurrentOp = LORAWAN_OP_NONE;
static uint8_t g_ActiveRxWindow = 0;
static bool g_RadioInitialized = false;
static bool g_Rx1Pending = false;
static bool g_Rx2Pending = false;
static uint32_t g_LastTxFrequency = 0;
static uint8_t g_LastTxDatarate = 0;
static uint8_t g_LastTxChannel = 0;

static void OnRadioTxDone(void);
static void OnRadioTxTimeout(void);
static void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void OnRadioRxTimeout(void);
static void OnRadioRxError(void);
static void OnRx1TimerEvent(void *context);
static void OnRx2TimerEvent(void *context);
static bool LoRaWAN_GetPhyParams(uint8_t dr, uint32_t *bandwidth, uint8_t *spreadingFactor);
static int8_t LoRaWAN_ComputeTxPowerDbm(uint8_t txPowerIndex, const LoRaWANRegionParams_t *region);
static void LoRaWAN_ResetRxTracking(void);
static void LoRaWAN_ScheduleRxWindows(LoRaWANContext_t *ctx);
static void LoRaWAN_OpenRxWindow(uint8_t window);
static void LoRaWAN_HandleRxWindowComplete(void);
static void LoRaWAN_HandleJoinFailure(void);
static LoRaWANStatus_t LoRaWAN_HandleJoinAccept(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size);

LoRaWANStatus_t LoRaWAN_Init(LoRaWANContext_t *ctx)
{
    if (ctx == NULL || ctx->Session == NULL || ctx->RadioBuffer == NULL || ctx->RadioBufferSize < 255)
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    g_ActiveCtx = ctx;
    ctx->Session->Joined = false;

    if (!g_RadioInitialized)
    {
        memset(&g_RadioEvents, 0, sizeof(g_RadioEvents));
        g_RadioEvents.TxDone = OnRadioTxDone;
        g_RadioEvents.TxTimeout = OnRadioTxTimeout;
        g_RadioEvents.RxDone = OnRadioRxDone;
        g_RadioEvents.RxTimeout = OnRadioRxTimeout;
        g_RadioEvents.RxError = OnRadioRxError;
        Radio.Init(&g_RadioEvents);
        Radio.SetPublicNetwork(true);

        TimerInit(&g_Rx1Timer, OnRx1TimerEvent);
        TimerInit(&g_Rx2Timer, OnRx2TimerEvent);
        TimerSetContext(&g_Rx1Timer, (void *)1);
        TimerSetContext(&g_Rx2Timer, (void *)2);

        g_RadioInitialized = true;
    }

    g_CurrentOp = LORAWAN_OP_NONE;
    LoRaWAN_ResetRxTracking();

    g_LastTxDatarate = ctx->Settings.DataRate;
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

    const LoRaWANRegionParams_t *region = LoRaWAN_RegionGetParams(ctx->Settings.Region);
    if (region == NULL || region->ChannelCount == 0)
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint8_t attempt = (uint8_t)ctx->Session->DevNonceCounter;
    uint32_t joinFrequency = LoRaWAN_RegionGetJoinFrequency(ctx->Settings.Region, attempt);
    if (joinFrequency == 0)
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint32_t bandwidth = 0;
    uint8_t spreadingFactor = 0;
    if (!LoRaWAN_GetPhyParams(ctx->Settings.DataRate, &bandwidth, &spreadingFactor))
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    LoRaWAN_ResetRxTracking();

    g_CurrentOp = LORAWAN_OP_JOIN;
    g_LastTxChannel = attempt % region->ChannelCount;
    g_LastTxFrequency = joinFrequency;
    g_LastTxDatarate = ctx->Settings.DataRate;

    Radio.SetModem(MODEM_LORA);
    Radio.SetChannel(joinFrequency);
    Radio.SetTxConfig(MODEM_LORA,
                      LoRaWAN_ComputeTxPowerDbm(ctx->Settings.TxPower, region),
                      0, bandwidth, spreadingFactor,
                      1, 8, false, true, 0, false, false, 4000);
    Radio.Send(frame, frameLen);

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

    const LoRaWANRegionParams_t *region = LoRaWAN_RegionGetParams(ctx->Settings.Region);
    if (region == NULL)
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint8_t channel = LoRaWAN_RegionGetNextChannel(ctx->Settings.Region, NULL);
    uint32_t uplinkFrequency = LoRaWAN_RegionGetUplinkFrequency(ctx->Settings.Region, channel);
    if (uplinkFrequency == 0)
    {
        return LORAWAN_STATUS_ERROR;
    }

    uint32_t bandwidth = 0;
    uint8_t spreadingFactor = 0;
    if (!LoRaWAN_GetPhyParams(ctx->Settings.DataRate, &bandwidth, &spreadingFactor))
    {
        return LORAWAN_STATUS_INVALID_PARAM;
    }

    LoRaWAN_ResetRxTracking();

    g_CurrentOp = LORAWAN_OP_TX;
    g_LastTxChannel = channel;
    g_LastTxFrequency = uplinkFrequency;
    g_LastTxDatarate = ctx->Settings.DataRate;

    Radio.SetModem(MODEM_LORA);
    Radio.SetChannel(uplinkFrequency);
    Radio.SetTxConfig(MODEM_LORA,
                      LoRaWAN_ComputeTxPowerDbm(ctx->Settings.TxPower, region),
                      0, bandwidth, spreadingFactor,
                      1, 8, false, true, 0, false, false, 3000);
    Radio.Send(frame, frameLen);

    ctx->Session->FCntUp++;
    Storage_UpdateFrameCounters(ctx->Session->FCntUp, ctx->Session->FCntDown);

    return LORAWAN_STATUS_SUCCESS;
}

void LoRaWAN_Process(LoRaWANContext_t *ctx)
{
    (void)ctx;
    TimerProcess();
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

static bool LoRaWAN_GetPhyParams(uint8_t dr, uint32_t *bandwidth, uint8_t *spreadingFactor)
{
    if (bandwidth == NULL || spreadingFactor == NULL)
    {
        return false;
    }

    switch (dr)
    {
        case 0:
            *bandwidth = 0;
            *spreadingFactor = 10;
            return true;
        case 1:
            *bandwidth = 0;
            *spreadingFactor = 9;
            return true;
        case 2:
            *bandwidth = 0;
            *spreadingFactor = 8;
            return true;
        case 3:
            *bandwidth = 0;
            *spreadingFactor = 7;
            return true;
        case 4:
            *bandwidth = 2;
            *spreadingFactor = 8;
            return true;
        case 5:
            *bandwidth = 2;
            *spreadingFactor = 7;
            return true;
        case 8:
            *bandwidth = 2;
            *spreadingFactor = 12;
            return true;
        case 9:
            *bandwidth = 2;
            *spreadingFactor = 11;
            return true;
        case 10:
            *bandwidth = 2;
            *spreadingFactor = 10;
            return true;
        case 11:
            *bandwidth = 2;
            *spreadingFactor = 9;
            return true;
        case 12:
            *bandwidth = 2;
            *spreadingFactor = 8;
            return true;
        case 13:
            *bandwidth = 2;
            *spreadingFactor = 7;
            return true;
        default:
            return false;
    }
}

static int8_t LoRaWAN_ComputeTxPowerDbm(uint8_t txPowerIndex, const LoRaWANRegionParams_t *region)
{
    int8_t maxEirp = (region != NULL) ? (int8_t)region->MaxEirp : 20;
    int8_t power = maxEirp - (int8_t)(txPowerIndex * 2);

    if (power < -4)
    {
        power = -4;
    }

    return power;
}

static void LoRaWAN_ResetRxTracking(void)
{
    TimerStop(&g_Rx1Timer);
    TimerStop(&g_Rx2Timer);
    g_Rx1Pending = false;
    g_Rx2Pending = false;
    g_ActiveRxWindow = 0;
}

static void LoRaWAN_ScheduleRxWindows(LoRaWANContext_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    uint32_t rx1Delay = (g_CurrentOp == LORAWAN_OP_JOIN) ? ctx->Settings.JoinRx1DelayMs : ctx->Settings.Rx1DelayMs;
    uint32_t rx2Delay = (g_CurrentOp == LORAWAN_OP_JOIN) ? ctx->Settings.JoinRx2DelayMs : ctx->Settings.Rx2DelayMs;

    if (rx1Delay == 0U)
    {
        rx1Delay = 1U;
    }
    if (rx2Delay == 0U)
    {
        rx2Delay = 1U;
    }

    if (rx1Delay > 0U)
    {
        TimerSetValue(&g_Rx1Timer, rx1Delay);
        TimerStart(&g_Rx1Timer);
        g_Rx1Pending = true;
    }

    if (rx2Delay > 0U)
    {
        TimerSetValue(&g_Rx2Timer, rx2Delay);
        TimerStart(&g_Rx2Timer);
        g_Rx2Pending = true;
    }
}

static void LoRaWAN_OpenRxWindow(uint8_t window)
{
    if (g_ActiveCtx == NULL)
    {
        return;
    }

    uint32_t frequency = 0;
    uint8_t datarate = 0;

    if (window == 1)
    {
        frequency = g_LastTxFrequency;
        datarate = g_LastTxDatarate;
    }
    else
    {
        frequency = g_ActiveCtx->Settings.Rx2Frequency;
        datarate = g_ActiveCtx->Settings.Rx2DataRate;
    }

    uint32_t bandwidth = 0;
    uint8_t spreadingFactor = 0;
    if (frequency == 0 || !LoRaWAN_GetPhyParams(datarate, &bandwidth, &spreadingFactor))
    {
        if (window == 1)
        {
            g_ActiveRxWindow = 0;
            return;
        }

        LoRaWAN_HandleRxWindowComplete();
        return;
    }

    Radio.SetModem(MODEM_LORA);
    Radio.SetChannel(frequency);
    Radio.SetRxConfig(MODEM_LORA, bandwidth, spreadingFactor, 1, 0, 8, 8, false, 0, true, 0, false, false, false);

    g_ActiveRxWindow = window;
    Radio.Rx(1000);
}

static void OnRx1TimerEvent(void *context)
{
    (void)context;
    g_Rx1Pending = false;
    LoRaWAN_OpenRxWindow(1);
}

static void OnRx2TimerEvent(void *context)
{
    (void)context;
    g_Rx2Pending = false;
    LoRaWAN_OpenRxWindow(2);
}

static void OnRadioTxDone(void)
{
    Radio.Standby();

    if (g_ActiveCtx == NULL)
    {
        return;
    }

    if (g_CurrentOp == LORAWAN_OP_NONE)
    {
        if (g_ActiveCtx->Callbacks.OnTxComplete != NULL)
        {
            g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SUCCESS);
        }
        return;
    }

    LoRaWAN_ScheduleRxWindows(g_ActiveCtx);
}

static void OnRadioTxTimeout(void)
{
    Radio.Standby();
    LoRaWAN_ResetRxTracking();

    if (g_CurrentOp == LORAWAN_OP_JOIN)
    {
        LoRaWAN_HandleJoinFailure();
        return;
    }

    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnTxComplete != NULL)
    {
        g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SEND_FAILED);
    }
    g_CurrentOp = LORAWAN_OP_NONE;
}

static LoRaWANStatus_t LoRaWAN_HandleJoinAccept(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size)
{
    if (ctx == NULL)
    {
        return LORAWAN_STATUS_ERROR;
    }

    return LoRaWAN_ParseJoinAccept(ctx, buffer, size);
}

static void LoRaWAN_HandleJoinFailure(void)
{
    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnJoinFailure != NULL)
    {
        g_ActiveCtx->Callbacks.OnJoinFailure();
    }

    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnTxComplete != NULL)
    {
        g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SEND_FAILED);
    }

    g_CurrentOp = LORAWAN_OP_NONE;
}

static void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    Radio.Standby();
    LoRaWAN_ResetRxTracking();

    if (g_CurrentOp == LORAWAN_OP_JOIN)
    {
        if (LoRaWAN_HandleJoinAccept(g_ActiveCtx, payload, (uint8_t)size) == LORAWAN_STATUS_SUCCESS)
        {
            g_CurrentOp = LORAWAN_OP_NONE;

            if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnTxComplete != NULL)
            {
                g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SUCCESS);
            }
        }
        else
        {
            LoRaWAN_HandleJoinFailure();
        }
        return;
    }

    g_CurrentOp = LORAWAN_OP_NONE;

    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnTxComplete != NULL)
    {
        g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SUCCESS);
    }

    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnRxData != NULL)
    {
        g_ActiveCtx->Callbacks.OnRxData(payload, (uint8_t)size, 0, rssi, snr);
    }
}

static void LoRaWAN_HandleRxWindowComplete(void)
{
    LoRaWAN_ResetRxTracking();

    if (g_CurrentOp == LORAWAN_OP_JOIN)
    {
        LoRaWAN_HandleJoinFailure();
        return;
    }

    if (g_ActiveCtx != NULL && g_ActiveCtx->Callbacks.OnTxComplete != NULL)
    {
        g_ActiveCtx->Callbacks.OnTxComplete(LORAWAN_STATUS_SUCCESS);
    }

    g_CurrentOp = LORAWAN_OP_NONE;
}

static void OnRadioRxTimeout(void)
{
    Radio.Standby();
    uint8_t window = g_ActiveRxWindow;
    g_ActiveRxWindow = 0;

    if (window == 1)
    {
        if (TimerIsStarted(&g_Rx2Timer) || g_Rx2Pending)
        {
            return;
        }
    }

    LoRaWAN_HandleRxWindowComplete();
}

static void OnRadioRxError(void)
{
    OnRadioRxTimeout();
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
