#include <string.h>
#include "lorawan_app.h"
#include "lorawan.h"
#include "config.h"
#include "storage.h"
#include "calibration.h"

#define DOWNLINK_MIN_SIZE 1

static LoRaWANStatus_t g_AppStatus = LORAWAN_STATUS_IDLE;
static LoRaWANContext_t g_LoRaCtx;
static LoRaWANSession_t g_Session;
static LoRaWANSettings_t g_Settings;
static uint8_t g_RadioBuffer[256];

static void OnJoinSuccess(uint32_t devAddr);
static void OnJoinFailure(void);
static void OnTxComplete(LoRaWANStatus_t status);
static void OnRxData(const uint8_t *buffer, uint8_t size, uint8_t port, int16_t rssi, int8_t snr);

static const LoRaWANCallbacks_t g_Callbacks = {
    .OnJoinSuccess = OnJoinSuccess,
    .OnJoinFailure = OnJoinFailure,
    .OnTxComplete = OnTxComplete,
    .OnRxData = OnRxData,
};

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
}

bool LoRaWANApp_Init(void)
{
    StorageData_t storage;

    if (!Storage_Init())
    {
        return false;
    }

    if (!Storage_Load(&storage))
    {
        memset(&storage, 0, sizeof(storage));
        storage.TxDutyCycle = LORAWAN_DEFAULT_TDC;
        storage.AdrEnabled = (LORAWAN_DEFAULT_ADR_STATE == LORAMAC_HANDLER_ADR_ON) ? 1 : 0;
        storage.DataRate = LORAWAN_DEFAULT_DATARATE;
        storage.TxPower = LORAWAN_DEFAULT_TX_POWER;
        storage.Rx2DataRate = LORAWAN_RX2_DATARATE;
        storage.Rx2Frequency = LORAWAN_RX2_FREQUENCY;
        storage.AppPort = LORAWAN_DEFAULT_APP_PORT;
    }

    LoRaWANApp_LoadSettings(&storage);

    g_LoRaCtx.Session = &g_Session;
    g_LoRaCtx.Settings = g_Settings;
    g_LoRaCtx.Callbacks = g_Callbacks;
    g_LoRaCtx.RadioBuffer = g_RadioBuffer;
    g_LoRaCtx.RadioBufferSize = sizeof(g_RadioBuffer);

    if (LoRaWAN_Init(&g_LoRaCtx) != LORAWAN_STATUS_SUCCESS)
    {
        return false;
    }

    g_AppStatus = g_Session.Joined ? LORAWAN_STATUS_JOINED : LORAWAN_STATUS_IDLE;
    return true;
}

bool LoRaWANApp_Join(void)
{
    g_AppStatus = LORAWAN_STATUS_JOINING;
    return (LoRaWAN_RequestJoin(&g_LoRaCtx) == LORAWAN_STATUS_SUCCESS);
}

bool LoRaWANApp_SendUplink(uint8_t *buffer, uint8_t size, uint8_t port, bool confirmed)
{
    LoRaWANMsgType_t type = confirmed ? LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED;
    LoRaWANStatus_t status = LoRaWAN_Send(&g_LoRaCtx, buffer, size, port, type);
    if (status == LORAWAN_STATUS_SUCCESS)
    {
        g_AppStatus = LORAWAN_STATUS_SENDING;
        return true;
    }
    return false;
}

void LoRaWANApp_Process(void)
{
    LoRaWAN_Process(&g_LoRaCtx);
}

LoRaWANStatus_t LoRaWANApp_GetStatus(void)
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
    g_AppStatus = LORAWAN_STATUS_JOINED;
}

static void OnJoinFailure(void)
{
    g_AppStatus = LORAWAN_STATUS_JOIN_FAILED;
}

static void OnTxComplete(LoRaWANStatus_t status)
{
    g_AppStatus = (status == LORAWAN_STATUS_SUCCESS) ? LORAWAN_STATUS_SEND_SUCCESS : LORAWAN_STATUS_SEND_FAILED;
}

static void HandleDownlinkOpcode(const uint8_t *payload, uint8_t size)
{
    if (size < DOWNLINK_MIN_SIZE)
    {
        return;
    }

    switch (payload[0])
    {
        case 0x01: /* Set TDC */
        {
            if (size >= 5)
            {
                uint32_t interval = 0;
                memcpy(&interval, &payload[1], sizeof(uint32_t));
                Storage_Write(STORAGE_KEY_TDC, (uint8_t *)&interval, sizeof(uint32_t));
                g_Settings.TxDutyCycleMs = interval;
                g_LoRaCtx.Settings.TxDutyCycleMs = interval;
            }
            break;
        }

        case 0x21: /* ADR */
        {
            if (size >= 2)
            {
                uint8_t adr = payload[1] ? 1 : 0;
                Storage_Write(STORAGE_KEY_ADR, &adr, 1);
                g_Settings.AdrState = adr ? LORAWAN_ADR_ON : LORAWAN_ADR_OFF;
                g_LoRaCtx.Settings.AdrState = g_Settings.AdrState;
            }
            break;
        }

        case 0x22: /* Data rate */
        {
            if (size >= 2)
            {
                uint8_t dr = payload[1];
                Storage_Write(STORAGE_KEY_DR, &dr, 1);
                g_Settings.DataRate = dr;
                g_LoRaCtx.Settings.DataRate = dr;
            }
            break;
        }

        case 0x23: /* TX power */
        {
            if (size >= 2)
            {
                uint8_t txp = payload[1];
                Storage_Write(STORAGE_KEY_TXP, &txp, 1);
                g_Settings.TxPower = txp;
                g_LoRaCtx.Settings.TxPower = txp;
            }
            break;
        }

        case 0xA0: /* Remote calibration */
        {
            uint8_t response[32];
            uint8_t responseSize = 0;
            if (Calibration_ProcessDownlink(payload, size, response, &responseSize))
            {
            }
            break;
        }

        default:
            break;
    }
}

static void OnRxData(const uint8_t *buffer, uint8_t size, uint8_t port, int16_t rssi, int8_t snr)
{
    (void)port;
    (void)rssi;
    (void)snr;
    HandleDownlinkOpcode(buffer, size);
}
