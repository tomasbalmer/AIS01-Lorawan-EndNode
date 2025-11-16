#include <string.h>
#include "lorawan_app.h"
#include "lorawan.h"
#include "config.h"
#include "storage.h"
#include "calibration.h"
#include "atcmd.h"
#include "sensor.h"
#include "uplink_encoder.h"
#include "downlink_dispatcher.h"
#include "board.h"
#include "hal_stubs.h"
#include "mac_mirror.h"
#include <stdio.h>

static LoRaWANAppState_t g_AppStatus = LORAWAN_APP_STATE_IDLE;
static LoRaWANContext_t g_LoRaCtx;
static LoRaWANSession_t g_Session;
static LoRaWANSettings_t g_Settings;
static uint8_t g_RadioBuffer[256];

static void OnJoinSuccess(uint32_t devAddr);
static void OnJoinFailure(void);
static void OnTxComplete(LoRaWANStatus_t status);
static void OnRxData(const uint8_t *buffer, uint8_t size, uint8_t port, int16_t rssi, int8_t snr);
static void Downlink_SetTdc(uint32_t interval);
static void Downlink_SetAdr(bool enabled);
static void Downlink_SetDataRate(uint8_t dr);
static void Downlink_SetTxPower(uint8_t txp);
static bool Downlink_ProcessCalibration(const uint8_t *payload, uint8_t size);

static const LoRaWANCallbacks_t g_Callbacks = {
    .OnJoinSuccess = OnJoinSuccess,
    .OnJoinFailure = OnJoinFailure,
    .OnTxComplete = OnTxComplete,
    .OnRxData = OnRxData};

static void LoRaWANApp_LoadSettings(const StorageData_t *storage)
{
    memcpy(g_Session.DevEui, storage->DevEui, sizeof(g_Session.DevEui));
    memcpy(g_Session.AppEui, storage->AppEui, sizeof(g_Session.AppEui));
    memcpy(g_Session.AppKey, storage->AppKey, sizeof(g_Session.AppKey));
    memcpy(g_Session.NwkSKey, storage->NwkSKey, sizeof(g_Session.NwkSKey));
    memcpy(g_Session.AppSKey, storage->AppSKey, sizeof(g_Session.AppSKey));
    g_Session.DevAddr = storage->DevAddr;
    g_Session.FCntUp = storage->FrameCounterUp;
    g_Session.FCntDown = storage->FrameCounterDown;
    g_Session.JoinMode = (LoRaWANJoinMode_t)storage->JoinMode;
    g_Session.DisableFrameCounterCheck = (storage->DisableFrameCounterCheck != 0);
    g_Session.Joined = (storage->DevAddr != 0);
    g_Session.DevNonceCounter = 0;

    g_Settings.Region = LORAWAN_REGION_AU915;
    g_Settings.DeviceClass = LORAWAN_DEVICE_CLASS_A;
    g_Settings.AdrState = storage->AdrEnabled ? LORAWAN_ADR_ON : LORAWAN_ADR_OFF;
    g_Settings.DataRate = storage->DataRate;
    g_Settings.TxPower = storage->TxPower;
    g_Settings.Rx2DataRate = storage->Rx2DataRate;
    g_Settings.Rx2Frequency = storage->Rx2Frequency;
    g_Settings.SubBand = storage->FreqBand;
    g_Settings.MsgType = storage->ConfirmedMsg ? LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED;
    g_Settings.AppPort = storage->AppPort;
    g_Settings.TxDutyCycleMs = storage->TxDutyCycle;
    g_Settings.Rx1DelayMs = storage->Rx1Delay;
    g_Settings.Rx2DelayMs = storage->Rx2Delay;
    g_Settings.JoinRx1DelayMs = storage->JoinRx1Delay;
    g_Settings.JoinRx2DelayMs = storage->JoinRx2Delay;
}

bool LoRaWANApp_Init(void)
{
    StorageData_t storage;

    StorageStatus_t loadStatus = Storage_Load(&storage);
    if (loadStatus != STORAGE_OK)
    {
        /* Storage_Load failed - use defaults */
        DEBUG_PRINT("LoRaWAN: Storage_Load failed (status=%d), using defaults\r\n", loadStatus);
        memset(&storage, 0, sizeof(storage));
        storage.TxDutyCycle = LORAWAN_DEFAULT_TDC;
        storage.AdrEnabled = (LORAWAN_DEFAULT_ADR_STATE != 0);
        storage.DataRate = LORAWAN_DEFAULT_DATARATE;
        storage.TxPower = LORAWAN_DEFAULT_TX_POWER;
        storage.Rx2DataRate = LORAWAN_RX2_DATARATE;
        storage.Rx2Frequency = LORAWAN_RX2_FREQUENCY;
        storage.AppPort = LORAWAN_DEFAULT_APP_PORT;
    }

    LoRaWANApp_LoadSettings(&storage);

    /* ABP mode activation */
    if (g_Session.JoinMode == LORAWAN_JOIN_MODE_ABP)
    {
        /* ABP mode: activate session immediately if DevAddr is configured */
        if (g_Session.DevAddr != 0)
        {
            g_Session.Joined = true;
            DEBUG_PRINT("ABP mode activated, DevAddr=0x%08lX\r\n", (unsigned long)g_Session.DevAddr);
        }
        else
        {
            g_Session.Joined = false;
            DEBUG_PRINT("ABP mode: DevAddr not configured\r\n");
        }
    }
    else
    {
        /* OTAA mode: wait for join */
        g_Session.Joined = false;
    }

    g_LoRaCtx.Session = &g_Session;
    g_LoRaCtx.Settings = g_Settings;
    g_LoRaCtx.Callbacks = g_Callbacks;
    g_LoRaCtx.RadioBuffer = g_RadioBuffer;
    g_LoRaCtx.RadioBufferSize = sizeof(g_RadioBuffer);

    if (LoRaWAN_Init(&g_LoRaCtx) != LORAWAN_STATUS_SUCCESS)
    {
        return false;
    }

    g_AppStatus = g_Session.Joined ? LORAWAN_APP_STATE_JOINED : LORAWAN_APP_STATE_IDLE;
    return true;
}

bool LoRaWANApp_Join(void)
{
    g_AppStatus = LORAWAN_APP_STATE_JOINING;
    return (LoRaWAN_RequestJoin(&g_LoRaCtx) == LORAWAN_STATUS_SUCCESS);
}

bool LoRaWANApp_SendUplink(uint8_t *buffer, uint8_t size, uint8_t port, bool confirmed)
{
    LoRaWANMsgType_t type = confirmed ? LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED;
    LoRaWANStatus_t status = LoRaWAN_Send(&g_LoRaCtx, buffer, size, port, type);
    if (status == LORAWAN_STATUS_SUCCESS)
    {
        g_AppStatus = LORAWAN_APP_STATE_SENDING;
        return true;
    }
    return false;
}

static bool LoRaWANApp_SendEncoded(const UplinkPayload_t *payload)
{
    if ((payload == NULL) || (payload->buffer == NULL) || (payload->size == 0U))
    {
        return false;
    }

    LoRaWANStatus_t status = LoRaWAN_Send(
        &g_LoRaCtx,
        payload->buffer,
        payload->size,
        g_Settings.AppPort,
        g_Settings.MsgType);

    if (status == LORAWAN_STATUS_SUCCESS)
    {
        g_AppStatus = LORAWAN_APP_STATE_SENDING;
        return true;
    }

    return false;
}

bool LoRaWANApp_SendStatusUplink(void)
{
    uint8_t buffer[16];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};
    UplinkStatusContext_t ctx;

    ctx.adrEnabled = (g_Settings.AdrState == LORAWAN_ADR_ON) ? 1U : 0U;
    ctx.dataRate = g_Settings.DataRate;
    ctx.txPower = g_Settings.TxPower;
    ctx.freqBand = g_Settings.SubBand;
    ctx.rssi = ATCmd_GetLastRSSI();
    ctx.snr = ATCmd_GetLastSNR();
    ctx.frameCounterUp = g_Session.FCntUp;
    ctx.batteryLevel = Sensor_GetBatteryLevel();

    if (!UplinkEncoder_EncodeStatus(&ctx, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

bool LoRaWANApp_SendCalibrationUplink(const uint8_t *calData, uint8_t calSize)
{
    uint8_t buffer[64];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    if ((calData == NULL) || (calSize == 0U))
    {
        return false;
    }

    if (!UplinkEncoder_EncodeCalibration(calData, calSize, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

bool LoRaWANApp_SendDebugUplink(uint8_t fwMajor,
                                uint8_t fwMinor,
                                uint8_t fwPatch,
                                uint8_t loraState)
{
    uint8_t buffer[16];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    if (!UplinkEncoder_EncodeDebug(fwMajor, fwMinor, fwPatch, loraState, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

bool LoRaWANApp_SendSensorUplink(void)
{
    uint8_t buffer[32];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    if (!Sensor_IsInitialized())
    {
        return false;
    }

    SensorSample_t sample;
    if (!Sensor_Read(&sample))
    {
        if (!Sensor_GetLastSample(&sample))
        {
            return false;
        }
    }

    uint8_t batt = Sensor_GetBatteryLevel();

    if (!UplinkEncoder_EncodeSensorFrame(&sample, batt, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

bool LoRaWANApp_SendSensorStatsUplink(void)
{
    uint8_t buffer[32];
    UplinkPayload_t p = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    UplinkSensorStatsContext_t ctx = {
        .batteryLevel = Sensor_GetBatteryLevel(),
        .primaryMin = 0U,
        .primaryAvg = 0U,
        .primaryMax = 0U,
        .secondaryMin = 0U,
        .secondaryAvg = 0U,
        .secondaryMax = 0U,
        .motionState = 0U,
        .occupancyState = 0U,
        .sensorMode = (uint8_t)Sensor_GetMode(),
        .timestampMs = HAL_GetTick()};

    if (!UplinkEncoder_EncodeSensorStats(&ctx, &p))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&p);
}

bool LoRaWANApp_SendStatusExUplink(void)
{
    uint8_t buffer[32];
    UplinkPayload_t p = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    UplinkStatusExContext_t ctx = {
        .adrEnabled = (g_Settings.AdrState == LORAWAN_ADR_ON) ? 1U : 0U,
        .dataRate = g_Settings.DataRate,
        .txPower = g_Settings.TxPower,
        .freqBand = g_Settings.SubBand,
        .rssi = ATCmd_GetLastRSSI(),
        .snr = ATCmd_GetLastSNR(),
        .batteryLevel = Sensor_GetBatteryLevel(),
        .batteryMv = BoardGetBatteryLevel(), /* Placeholder until real mV routine */
        .sensorPowered = Sensor_IsPowered() ? 1U : 0U,
        .sensorMode = (uint8_t)Sensor_GetMode(),
        .uptimeSec = HAL_GetTick() / 1000U,
        .frameCounterUp = g_Session.FCntUp,
        .pendingDl = ATCmd_GetPendingDownlink()};

    if (!UplinkEncoder_EncodeStatusEx(&ctx, &p))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&p);
}

bool LoRaWANApp_SendMacMirrorUplink(void)
{
    MacMirrorFrame_t frame;

    if (!MacMirror_GetLast(&frame))
    {
        return false;
    }

    uint8_t buffer[32];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    UplinkMacMirrorContext_t ctx;
    memcpy(ctx.payload, frame.buffer, frame.size);
    ctx.size = frame.size;

    if (!UplinkEncoder_EncodeMacMirror(&ctx, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

bool LoRaWANApp_SendPowerProfileUplink(void)
{
    uint8_t buffer[16];
    UplinkPayload_t payload = {
        .buffer = buffer,
        .maxSize = (uint8_t)sizeof(buffer),
        .size = 0U};

    UplinkPowerProfileContext_t ctx = {
        .batteryLevel = Sensor_GetBatteryLevel(),
        .batteryMv = BoardGetBatteryLevel(),
        .uptimeSec = HAL_GetTick() / 1000U,
        .sensorPowered = Sensor_IsPowered() ? 1U : 0U,
        .dataRate = g_Settings.DataRate};

    if (!UplinkEncoder_EncodePowerProfile(&ctx, &payload))
    {
        return false;
    }

    return LoRaWANApp_SendEncoded(&payload);
}

void LoRaWANApp_Process(void)
{
    LoRaWAN_Process(&g_LoRaCtx);
}

LoRaWANAppState_t LoRaWANApp_GetStatus(void)
{
    return g_AppStatus;
}

bool LoRaWANApp_IsJoined(void)
{
    return g_Session.Joined;
}

uint32_t LoRaWANApp_GetDevAddr(void)
{
    return g_Session.DevAddr;
}

static void OnJoinSuccess(uint32_t devAddr)
{
    g_Session.Joined = true;
    g_Session.DevAddr = devAddr;
    g_Session.FCntUp = 0;
    g_Session.FCntDown = 0;
    Storage_UpdateJoinKeys(devAddr, g_Session.NwkSKey, g_Session.AppSKey);
    g_AppStatus = LORAWAN_APP_STATE_JOINED;
}

static void OnJoinFailure(void)
{
    g_AppStatus = LORAWAN_APP_STATE_JOIN_FAILED;
}

static void OnTxComplete(LoRaWANStatus_t status)
{
    g_AppStatus = (status == LORAWAN_STATUS_SUCCESS) ? LORAWAN_APP_STATE_SEND_SUCCESS : LORAWAN_APP_STATE_SEND_FAILED;

    /* Update confirmed message status for AT commands */
    ATCmd_UpdateConfirmedStatus(status == LORAWAN_STATUS_SUCCESS ? 1 : 2);
}

static void Downlink_SetTdc(uint32_t interval)
{
    Storage_Write(STORAGE_KEY_TDC, (const uint8_t *)&interval, sizeof(interval));
    g_Settings.TxDutyCycleMs = interval;
    g_LoRaCtx.Settings.TxDutyCycleMs = interval;
}

static void Downlink_SetAdr(bool enabled)
{
    uint8_t adr = enabled ? 1U : 0U;
    Storage_Write(STORAGE_KEY_ADR, &adr, 1U);
    g_Settings.AdrState = enabled ? LORAWAN_ADR_ON : LORAWAN_ADR_OFF;
    g_LoRaCtx.Settings.AdrState = g_Settings.AdrState;
}

static void Downlink_SetDataRate(uint8_t dr)
{
    Storage_Write(STORAGE_KEY_DR, &dr, 1U);
    g_Settings.DataRate = dr;
    g_LoRaCtx.Settings.DataRate = dr;
}

static void Downlink_SetTxPower(uint8_t txp)
{
    Storage_Write(STORAGE_KEY_TXP, &txp, 1U);
    g_Settings.TxPower = txp;
    g_LoRaCtx.Settings.TxPower = txp;
}

static bool Downlink_ProcessCalibration(const uint8_t *payload, uint8_t size)
{
    uint8_t response[CALIBRATION_BUFFER_SIZE];
    uint8_t responseSize = 0;
    return Calibration_ProcessDownlink(payload, size, response, &responseSize);
}

static void OnRxData(const uint8_t *buffer, uint8_t size, uint8_t port, int16_t rssi, int8_t snr)
{

    /* Update RSSI and SNR for AT commands */
    ATCmd_UpdateRSSI(rssi);
    ATCmd_UpdateSNR(snr);
    ATCmd_UpdatePendingDownlink(size > 0 ? 1 : 0);

    uint8_t opcode = (size > 0U) ? buffer[0] : 0U;
    DownlinkContext_t ctx = {
        .opcode = opcode,
        .length = size,
        .rssi = rssi,
        .snr = snr};

    DownlinkActions_t actions = {
        .setTdc = Downlink_SetTdc,
        .setAdr = Downlink_SetAdr,
        .setDataRate = Downlink_SetDataRate,
        .setTxPower = Downlink_SetTxPower,
        .processCalibration = Downlink_ProcessCalibration};

    if ((port == 0U) && (size > 0U))
    {
        MacMirror_StoreRx(buffer, size);
    }

    Downlink_Handle(buffer, size, &ctx, &actions);
}
