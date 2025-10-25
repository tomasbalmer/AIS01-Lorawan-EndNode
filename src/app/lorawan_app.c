/*!
 * \file      lorawan_app.c
 *
 * \brief     LoRaWAN application implementation
 *
 * \details   Integrates with LoRaMAC stack, handles join/uplink/downlink,
 *            and processes calibration commands from downlinks.
 */
#include "lorawan_app.h"
#include "config.h"
#include "storage.h"
#include "calibration.h"
#include "LoRaMac.h"
#include "LmHandler.h"
#include "board.h"
#include "RegionCommon.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static LoRaWANStatus_t g_Status = LORAWAN_STATUS_IDLE;
static bool g_LoRaWANInitialized = false;
static uint8_t g_AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
static LmHandlerAppData_t g_AppData = {
    .Buffer = g_AppDataBuffer,
    .BufferSize = 0,
    .Port = LORAWAN_DEFAULT_APP_PORT,
};

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES - CALLBACKS
 * ========================================================================== */
static void OnMacProcessNotify(void);
static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size);
static void OnNetworkParametersChange(CommissioningParams_t *params);
static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn);
static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn);
static void OnJoinRequest(LmHandlerJoinParams_t *params);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection);

/* ============================================================================
 * LORAWAN HANDLER CALLBACKS
 * ========================================================================== */
static LmHandlerCallbacks_t g_LmHandlerCallbacks = {
    .GetBatteryLevel = BoardGetBatteryLevel,
    .GetTemperature = NULL,
    .GetRandomSeed = BoardGetRandomSeed,
    .OnMacProcess = OnMacProcessNotify,
    .OnNvmDataChange = OnNvmDataChange,
    .OnNetworkParametersChange = OnNetworkParametersChange,
    .OnMacMcpsRequest = OnMacMcpsRequest,
    .OnMacMlmeRequest = OnMacMlmeRequest,
    .OnJoinRequest = OnJoinRequest,
    .OnTxData = OnTxData,
    .OnRxData = OnRxData,
    .OnClassChange = OnClassChange,
    .OnBeaconStatusChange = OnBeaconStatusChange,
    .OnSysTimeUpdate = OnSysTimeUpdate,
};

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool LoRaWANApp_Init(void)
{
    if (g_LoRaWANInitialized)
    {
        return true;
    }

    /* Load configuration from storage */
    StorageData_t storage;
    if (!Storage_Load(&storage))
    {
        DEBUG_PRINT("Failed to load storage configuration\r\n");
        return false;
    }

    /* Configure LmHandler parameters */
    LmHandlerParams_t handlerParams = {
        .Region = ACTIVE_REGION,
        .AdrEnable = storage.AdrEnabled ? LORAMAC_HANDLER_ADR_ON : LORAMAC_HANDLER_ADR_OFF,
        .IsTxConfirmed = storage.ConfirmedMsg ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG,
        .TxDatarate = storage.DataRate,
        .PublicNetworkEnable = true,
        .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
        .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
        .DataBuffer = g_AppDataBuffer,
        .PingSlotPeriodicity = REGION_COMMON_DEFAULT_PING_SLOT_PERIODICITY,
    };

    /* Initialize LoRaMac handler */
    if (LmHandlerInit(&g_LmHandlerCallbacks, &handlerParams) != LORAMAC_HANDLER_SUCCESS)
    {
        DEBUG_PRINT("LoRaMac handler initialization failed\r\n");
        return false;
    }

    /* Set system maximum tolerated RX error */
    LmHandlerSetSystemMaxRxError(20);

    /* Configure AU915 sub-band */
    if (ACTIVE_REGION == LORAMAC_REGION_AU915)
    {
        /* AU915 Sub-band 2 (channels 8-15) */
        uint16_t channelMask[] = {0x0000, 0x00FF, 0x0000, 0x0000, 0x0000};
        LoRaMacChannelsMask_t mask;
        memcpy(mask.ChannelsMask, channelMask, sizeof(channelMask));
        /* Note: Use proper LoRaMAC API to set channel mask */
    }

    g_LoRaWANInitialized = true;
    DEBUG_PRINT("LoRaWAN initialized (AU915, Sub-band %d)\r\n", LORAWAN_AU915_SUB_BAND);

    return true;
}

bool LoRaWANApp_Join(void)
{
    if (!g_LoRaWANInitialized)
    {
        return false;
    }

    g_Status = LORAWAN_STATUS_JOINING;
    DEBUG_PRINT("Initiating OTAA join...\r\n");

    LmHandlerJoin();

    return true;
}

bool LoRaWANApp_SendUplink(uint8_t *buffer, uint8_t size, uint8_t port, bool confirmed)
{
    if (!g_LoRaWANInitialized || !LoRaWANApp_IsJoined())
    {
        return false;
    }

    if (LmHandlerIsBusy())
    {
        DEBUG_PRINT("LoRaMAC is busy, cannot send\r\n");
        return false;
    }

    /* Prepare uplink data */
    g_AppData.Port = port;
    g_AppData.BufferSize = size;
    memcpy(g_AppData.Buffer, buffer, size);

    g_Status = LORAWAN_STATUS_SENDING;

    LmHandlerMsgTypes_t msgType = confirmed ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG;

    if (LmHandlerSend(&g_AppData, msgType) == LORAMAC_HANDLER_SUCCESS)
    {
        DEBUG_PRINT("Uplink queued (port %d, size %d)\r\n", port, size);
        return true;
    }

    g_Status = LORAWAN_STATUS_SEND_FAILED;
    return false;
}

void LoRaWANApp_Process(void)
{
    if (!g_LoRaWANInitialized)
    {
        return;
    }

    /* Process LoRaMac events */
    LmHandlerProcess();
}

LoRaWANStatus_t LoRaWANApp_GetStatus(void)
{
    return g_Status;
}

bool LoRaWANApp_IsJoined(void)
{
    return (g_Status == LORAWAN_STATUS_JOINED ||
            g_Status == LORAWAN_STATUS_SENDING ||
            g_Status == LORAWAN_STATUS_SEND_SUCCESS ||
            g_Status == LORAWAN_STATUS_SEND_FAILED);
}

uint32_t LoRaWANApp_GetDevAddr(void)
{
    /* Return DevAddr from LoRaMac context */
    /* TODO: Implement using LmHandler API */
    return 0;
}

/* ============================================================================
 * PRIVATE CALLBACK IMPLEMENTATIONS
 * ========================================================================== */

static void OnMacProcessNotify(void)
{
    /* MAC process notification - wake up from low power mode */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size)
{
    DEBUG_PRINT("NVM data change: state=%d, size=%d\r\n", state, size);
}

static void OnNetworkParametersChange(CommissioningParams_t *params)
{
    DEBUG_PRINT("Network parameters updated\r\n");
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn)
{
    DEBUG_PRINT("MCPS Request: status=%d, nextTx=%lu\r\n", status, nextTxIn);
}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn)
{
    DEBUG_PRINT("MLME Request: status=%d, nextTx=%lu\r\n", status, nextTxIn);
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
    if (params->Status == LORAMAC_HANDLER_ERROR)
    {
        DEBUG_PRINT("Join failed, retrying...\r\n");
        g_Status = LORAWAN_STATUS_JOIN_FAILED;

        /* Retry join */
        LmHandlerJoin();
    }
    else if (params->Status == LORAMAC_HANDLER_SUCCESS)
    {
        DEBUG_PRINT("Join successful!\r\n");
        g_Status = LORAWAN_STATUS_JOINED;

        /* Set device class (Class A by default) */
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
    }
}

static void OnTxData(LmHandlerTxParams_t *params)
{
    DEBUG_PRINT("TX Done: datarate=%d, txPower=%d\r\n", params->Datarate, params->TxPower);

    if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
    {
        if (params->AckReceived)
        {
            DEBUG_PRINT("ACK received\r\n");
            g_Status = LORAWAN_STATUS_SEND_SUCCESS;
        }
        else
        {
            DEBUG_PRINT("ACK not received\r\n");
            g_Status = LORAWAN_STATUS_SEND_FAILED;
        }
    }
    else
    {
        g_Status = LORAWAN_STATUS_SEND_SUCCESS;
    }

    /* After TX, return to joined state */
    g_Status = LORAWAN_STATUS_JOINED;
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    DEBUG_PRINT("RX: port=%d, datarate=%d, rssi=%d, snr=%d, size=%d\r\n",
                appData->Port, params->Datarate, params->Rssi, params->Snr, appData->BufferSize);

    /* Check if this is a calibration command */
    if (appData->Port == CALIBRATION_DOWNLINK_PORT && appData->BufferSize > 0)
    {
        if (appData->Buffer[0] == CALIBRATION_OPCODE)
        {
            DEBUG_PRINT("Processing calibration command\r\n");

            uint8_t response[32];
            uint8_t responseSize = 0;

            if (Calibration_ProcessDownlink(&appData->Buffer[1], appData->BufferSize - 1,
                                           response, &responseSize))
            {
                /* Send calibration response as uplink */
                if (responseSize > 0)
                {
                    LoRaWANApp_SendUplink(response, responseSize, CALIBRATION_DOWNLINK_PORT, false);
                }
            }
        }
    }
    else
    {
        /* Handle other downlink ports */
        DEBUG_PRINT("Downlink received on port %d\r\n", appData->Port);
    }
}

static void OnClassChange(DeviceClass_t deviceClass)
{
    DEBUG_PRINT("Device class changed to %d\r\n", deviceClass);
}

static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params)
{
    /* Class B beacon status - not used in Class A */
}

static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection)
{
    if (isSynchronized)
    {
        DEBUG_PRINT("System time synchronized (correction: %ld ms)\r\n", timeCorrection);
    }
}
