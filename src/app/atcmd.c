/*!
 * \file      atcmd.c
 *
 * \brief     AT command parser implementation
 *
 * \details   Command table and handlers for LoRaWAN configuration via AT commands.
 */
#include "atcmd.h"
#include "config.h"
#include "storage.h"
#include "calibration.h"
#include "lorawan_app.h"
#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* ============================================================================
 * PRIVATE DEFINITIONS
 * ========================================================================== */
#define ATCMD_BUFFER_SIZE           AT_CMD_MAX_LENGTH
#define ATCMD_MAX_ARGS              8

/* Standard AT Responses (match original firmware) */
#define ATCMD_RESP_OK               "OK\r\n"
#define ATCMD_RESP_ERROR            "AT_ERROR\r\n"
#define ATCMD_RESP_PARAM_ERROR      "AT_PARAM_ERROR\r\n"
#define ATCMD_RESP_BUSY_ERROR       "AT_BUSY_ERROR\r\n"
#define ATCMD_RESP_NO_NET_JOINED    "AT_NO_NET_JOINED\r\n"
#define ATCMD_RESP_JOINED           "ABC JOINED\r\n"  /* String at 0x08013349 */

/* TDC Validation (from original firmware at 0x08013A7F) */
#define TDC_MINIMUM_MS              4000
#define TDC_ERROR_MSG               "TDC setting needs to be high than 4000ms\r\n"

/* ============================================================================
 * PRIVATE TYPES
 * ========================================================================== */
typedef ATCmdResult_t (*ATCmdHandler_t)(int argc, char *argv[]);

typedef struct
{
    const char *name;
    ATCmdHandler_t handler;
    const char *help;
} ATCmdEntry_t;

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static char g_CmdBuffer[ATCMD_BUFFER_SIZE];
static uint16_t g_CmdBufferIndex = 0;
static bool g_ATCmdInitialized = false;
static int16_t g_LastRSSI = 0;
static int8_t g_LastSNR = 0;
static uint8_t g_PendingDownlink = 0;
static uint8_t g_LastConfirmedStatus = 0;  /* 0=pending, 1=success, 2=failed */
static uint32_t g_UTCTimeSeconds = 0;  /* UTC time in seconds since epoch */
static uint16_t g_ChannelMask = 0x00FF;  /* Default: channels 0-7 enabled (sub-band 2) */
static uint32_t g_WakeupInterval = 60000;  /* Default: 60 seconds */
static uint8_t g_PlatformData[32] = {0};  /* Custom platform data */
static uint8_t g_UserSettings[32] = {0};  /* User-defined settings */
static uint8_t g_SensorInitialized = 0;  /* Sensor initialization status */
static uint8_t g_SensorMode = 0;  /* Sensor operation mode */
static uint8_t g_SensorPowerOn = 0;  /* Sensor power status */
static uint8_t g_LowPowerEnabled = 1;  /* Low power mode enabled */
static uint8_t g_DutyCycleEnabled = 0;  /* Duty cycle enforcement (AU915 doesn't require it) */
static uint8_t g_TestModeEnabled = 0;  /* RF test mode status */
static uint8_t g_LoggingEnabled = 0;  /* Logging enable status */

/* ============================================================================
 * AT COMMAND HANDLERS - PROTOTYPES
 * ========================================================================== */
static ATCmdResult_t ATCmd_HandleTest(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleVersion(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleReset(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleFactoryReset(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDevEUI(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleAppEUI(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleAppKey(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleJoin(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleADR(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDataRate(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTxPower(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTDC(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandlePort(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleConfirmed(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRX2DR(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRX2FQ(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleFreqBand(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleBattery(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDebug(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleCalibRemote(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleNetworkJoinMode(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleNwkSKey(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleAppSKey(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDevAddr(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDisableFrameCounterCheck(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleNetworkJoinStatus(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRX1DL(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRX2DL(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleJRX1DL(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleJRX2DL(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleFrameCounterUp(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleFrameCounterDown(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRSSI(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSNR(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleClass(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRecv(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRetry(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleRetryDelay(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSend(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSendBinary(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleConfirmedMode(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleConfirmedStatus(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleAppPort(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTimeRequest(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleLocalTime(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleUTC(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleChannelEnable(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleChannelSingle(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleChannel(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleBandPlan(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleChannelMask(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTemperature(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleVoltage(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleChipID(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSleep(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleWakeup(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandlePlatformData(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleUserSettings(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSensorInit(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSensorRead(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSensorCalibrate(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSensorMode(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleSensorPower(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleLowPower(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleDutyCycle(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleLinkCheck(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestTxCW(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestConfig(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestOff(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestOn(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestTone(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestRx(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestTx(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleTestRSSI(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleLog(int argc, char *argv[]);
static ATCmdResult_t ATCmd_HandleHelp(int argc, char *argv[]);

/* ============================================================================
 * AT COMMAND TABLE
 * ========================================================================== */
static const ATCmdEntry_t g_ATCmdTable[] = {
    /* Basic commands */
    { "AT", ATCmd_HandleTest, "Test command" },
    { "AT+VER", ATCmd_HandleVersion, "Get firmware version" },
    { "ATZ", ATCmd_HandleReset, "Reset device" },
    { "AT+FDR", ATCmd_HandleFactoryReset, "Factory reset" },

    /* LoRaWAN credentials */
    { "AT+DEVEUI", ATCmd_HandleDevEUI, "Get/Set DevEUI (8 bytes hex)" },
    { "AT+APPEUI", ATCmd_HandleAppEUI, "Get/Set AppEUI (8 bytes hex)" },
    { "AT+APPKEY", ATCmd_HandleAppKey, "Get/Set AppKey (16 bytes hex)" },

    /* LoRaWAN configuration */
    { "AT+JOIN", ATCmd_HandleJoin, "Join network (OTAA)" },
    { "AT+ADR", ATCmd_HandleADR, "Get/Set ADR enable (0/1)" },
    { "AT+DR", ATCmd_HandleDataRate, "Get/Set data rate (0-5)" },
    { "AT+TXP", ATCmd_HandleTxPower, "Get/Set TX power (0-10)" },
    { "AT+TDC", ATCmd_HandleTDC, "Get/Set TX duty cycle (ms)" },
    { "AT+PORT", ATCmd_HandlePort, "Get/Set application port (1-223)" },
    { "AT+PNACKMD", ATCmd_HandleConfirmed, "Get/Set confirmed mode (0/1)" },
    { "AT+RX2DR", ATCmd_HandleRX2DR, "Get/Set RX2 data rate" },
    { "AT+RX2FQ", ATCmd_HandleRX2FQ, "Get/Set RX2 frequency (Hz)" },
    { "AT+FREQBAND", ATCmd_HandleFreqBand, "Get/Set frequency sub-band (1-9)" },

    /* ABP Mode Support */
    { "AT+NJM", ATCmd_HandleNetworkJoinMode, "Get/Set join mode (0=ABP, 1=OTAA)" },
    { "AT+NWKSKEY", ATCmd_HandleNwkSKey, "Get/Set Network Session Key (16 bytes hex)" },
    { "AT+APPSKEY", ATCmd_HandleAppSKey, "Get/Set App Session Key (16 bytes hex)" },
    { "AT+DADDR", ATCmd_HandleDevAddr, "Get/Set Device Address (4 bytes hex)" },
    { "AT+DISFCNTCHECK", ATCmd_HandleDisableFrameCounterCheck, "Disable frame counter check (0/1)" },
    { "AT+NJS", ATCmd_HandleNetworkJoinStatus, "Get network join status (read-only)" },

    /* RX Window Configuration */
    { "AT+RX1DL", ATCmd_HandleRX1DL, "Get/Set RX1 delay (ms)" },
    { "AT+RX2DL", ATCmd_HandleRX2DL, "Get/Set RX2 delay (ms)" },
    { "AT+JRX1DL", ATCmd_HandleJRX1DL, "Get/Set Join RX1 delay (ms)" },
    { "AT+JRX2DL", ATCmd_HandleJRX2DL, "Get/Set Join RX2 delay (ms)" },

    /* Frame Counters */
    { "AT+FCU", ATCmd_HandleFrameCounterUp, "Get uplink frame counter (read-only)" },
    { "AT+FCD", ATCmd_HandleFrameCounterDown, "Get downlink frame counter (read-only)" },

    /* Signal Quality */
    { "AT+RSSI", ATCmd_HandleRSSI, "Get last RSSI (read-only)" },
    { "AT+SNR", ATCmd_HandleSNR, "Get last SNR (read-only)" },

    /* Device Configuration */
    { "AT+CLASS", ATCmd_HandleClass, "Get/Set device class (A/B/C)" },
    { "AT+RECV", ATCmd_HandleRecv, "Check for pending downlink data" },
    { "AT+RETY", ATCmd_HandleRetry, "Get/Set retry count for confirmed messages" },
    { "AT+DELAY", ATCmd_HandleRetryDelay, "Get/Set delay between retries (ms)" },

    /* Data Transmission */
    { "AT+SEND", ATCmd_HandleSend, "Send uplink data (hex string)" },
    { "AT+SENDB", ATCmd_HandleSendBinary, "Send binary uplink data" },
    { "AT+CFM", ATCmd_HandleConfirmedMode, "Get/Set confirmed mode (0/1)" },
    { "AT+CFS", ATCmd_HandleConfirmedStatus, "Get last confirmed message status" },
    { "AT+APPPORT", ATCmd_HandleAppPort, "Get/Set application port (alias)" },

    /* Time Synchronization */
    { "AT+TIMEREQ", ATCmd_HandleTimeRequest, "Request time synchronization" },
    { "AT+LTIME", ATCmd_HandleLocalTime, "Get local time" },
    { "AT+UTC", ATCmd_HandleUTC, "Get/Set UTC time" },

    /* Channel Configuration */
    { "AT+CHE", ATCmd_HandleChannelEnable, "Get/Set channel enable mask" },
    { "AT+CHS", ATCmd_HandleChannelSingle, "Get/Set single channel parameters" },
    { "AT+CH", ATCmd_HandleChannel, "Get channel configuration" },
    { "AT+BAND", ATCmd_HandleBandPlan, "Get/Set band plan" },
    { "AT+MASK", ATCmd_HandleChannelMask, "Get/Set channel mask" },

    /* System commands */
    { "AT+BAT", ATCmd_HandleBattery, "Get battery level" },
    { "AT+TEMP", ATCmd_HandleTemperature, "Get temperature (°C)" },
    { "AT+VDD", ATCmd_HandleVoltage, "Get supply voltage (mV)" },
    { "AT+ID", ATCmd_HandleChipID, "Get chip unique ID" },
    { "AT+SLEEP", ATCmd_HandleSleep, "Enter sleep mode" },
    { "AT+WAKEUP", ATCmd_HandleWakeup, "Get/Set wakeup interval (ms)" },
    { "AT+PDTA", ATCmd_HandlePlatformData, "Get/Set platform data" },
    { "AT+USET", ATCmd_HandleUserSettings, "Get/Set user settings" },
    { "AT+DEBUG", ATCmd_HandleDebug, "Get/Set debug mode (0/1)" },

    /* Sensor Integration */
    { "AT+SENSORINIT", ATCmd_HandleSensorInit, "Initialize sensor" },
    { "AT+SENSORREAD", ATCmd_HandleSensorRead, "Read sensor data" },
    { "AT+SENSORCAL", ATCmd_HandleSensorCalibrate, "Calibrate sensor" },
    { "AT+SENSORMODE", ATCmd_HandleSensorMode, "Get/Set sensor mode" },
    { "AT+SENSORPWR", ATCmd_HandleSensorPower, "Get/Set sensor power" },

    /* Power Management */
    { "AT+LOWPOWER", ATCmd_HandleLowPower, "Get/Set low power mode" },
    { "AT+DUTYCYCLE", ATCmd_HandleDutyCycle, "Get/Set duty cycle enable" },

    /* Network Testing */
    { "AT+LINKCHECK", ATCmd_HandleLinkCheck, "Request link check" },

    /* RF Testing */
    { "AT+TXCW", ATCmd_HandleTestTxCW, "TX continuous wave test" },
    { "AT+TCONF", ATCmd_HandleTestConfig, "Configure test mode" },
    { "AT+TOFF", ATCmd_HandleTestOff, "Disable test mode" },
    { "AT+TON", ATCmd_HandleTestOn, "Enable test mode" },
    { "AT+TTONE", ATCmd_HandleTestTone, "Transmit tone" },
    { "AT+TRX", ATCmd_HandleTestRx, "Test RX mode" },
    { "AT+TTX", ATCmd_HandleTestTx, "Test TX mode" },
    { "AT+TRSSI", ATCmd_HandleTestRSSI, "Test RSSI measurement" },

    /* Utility */
    { "AT+LOG", ATCmd_HandleLog, "Get/Set logging enable" },
    { "AT+HELP", ATCmd_HandleHelp, "List all commands" },

    /* Calibration */
    { "AT+CALIBREMOTE", ATCmd_HandleCalibRemote, "Remote calibration command" },
};

#define ATCMD_TABLE_SIZE    (sizeof(g_ATCmdTable) / sizeof(ATCmdEntry_t))

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static void ATCmd_ParseCommand(char *cmdLine, int *argc, char *argv[]);
static uint8_t ATCmd_HexCharToNibble(char c);
static bool ATCmd_HexStringToBytes(const char *hexStr, uint8_t *bytes, uint8_t len);
static void ATCmd_BytesToHexString(const uint8_t *bytes, uint8_t len, char *hexStr);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool ATCmd_Init(void)
{
    if (g_ATCmdInitialized)
    {
        return true;
    }

    memset(g_CmdBuffer, 0, ATCMD_BUFFER_SIZE);
    g_CmdBufferIndex = 0;
    g_ATCmdInitialized = true;

    ATCmd_SendResponse("\r\n+READY\r\n");
    return true;
}

ATCmdResult_t ATCmd_Process(const char *cmdLine)
{
    if (cmdLine == NULL || strlen(cmdLine) == 0)
    {
        return ATCMD_ERROR;
    }

    /* Parse command line */
    int argc = 0;
    char *argv[ATCMD_MAX_ARGS];
    char cmdLineCopy[ATCMD_BUFFER_SIZE];
    strncpy(cmdLineCopy, cmdLine, ATCMD_BUFFER_SIZE - 1);
    ATCmd_ParseCommand(cmdLineCopy, &argc, argv);

    if (argc == 0)
    {
        return ATCMD_ERROR;
    }

    /* Find and execute command handler */
    for (uint32_t i = 0; i < ATCMD_TABLE_SIZE; i++)
    {
        if (strncmp(argv[0], g_ATCmdTable[i].name, strlen(g_ATCmdTable[i].name)) == 0)
        {
            return g_ATCmdTable[i].handler(argc, argv);
        }
    }

    ATCmd_SendResponse("ERROR: Unknown command\r\n");
    return ATCMD_NOT_FOUND;
}

void ATCmd_ProcessChar(uint8_t rxChar)
{
    if (!g_ATCmdInitialized)
    {
        return;
    }

    /* Echo character if enabled */
#if AT_ECHO_ENABLED
    printf("%c", rxChar);
#endif

    /* Handle special characters */
    if (rxChar == '\r' || rxChar == '\n')
    {
        if (g_CmdBufferIndex > 0)
        {
            g_CmdBuffer[g_CmdBufferIndex] = '\0';
            ATCmdResult_t result = ATCmd_Process(g_CmdBuffer);

            if (result == ATCMD_OK)
            {
                ATCmd_SendResponse("OK\r\n");
            }
            else if (result != ATCMD_NOT_FOUND)
            {
                ATCmd_SendResponse("ERROR\r\n");
            }

            g_CmdBufferIndex = 0;
            memset(g_CmdBuffer, 0, ATCMD_BUFFER_SIZE);
        }
        return;
    }

    /* Handle backspace */
    if (rxChar == '\b' || rxChar == 0x7F)
    {
        if (g_CmdBufferIndex > 0)
        {
            g_CmdBufferIndex--;
        }
        return;
    }

    /* Add character to buffer */
    if (g_CmdBufferIndex < ATCMD_BUFFER_SIZE - 1)
    {
        g_CmdBuffer[g_CmdBufferIndex++] = rxChar;
    }
}

void ATCmd_SendResponse(const char *response)
{
    printf("%s", response);
}

void ATCmd_SendFormattedResponse(const char *format, ...)
{
    char buffer[AT_RESPONSE_MAX_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, AT_RESPONSE_MAX_LENGTH, format, args);
    va_end(args);
    ATCmd_SendResponse(buffer);
}

void ATCmd_UpdateRSSI(int16_t rssi)
{
    g_LastRSSI = rssi;
}

void ATCmd_UpdateSNR(int8_t snr)
{
    g_LastSNR = snr;
}

void ATCmd_UpdatePendingDownlink(uint8_t pending)
{
    g_PendingDownlink = pending;
}

void ATCmd_UpdateConfirmedStatus(uint8_t status)
{
    g_LastConfirmedStatus = status;
}

/* ============================================================================
 * AT COMMAND HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleTest(int argc, char *argv[])
{
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleVersion(int argc, char *argv[])
{
    ATCmd_SendFormattedResponse("+VER: %d.%d.%d\r\n",
        FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleReset(int argc, char *argv[])
{
    ATCmd_SendResponse("Resetting...\r\n");
    HAL_Delay(100);
    NVIC_SystemReset();
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleFactoryReset(int argc, char *argv[])
{
    if (Storage_FactoryReset())
    {
        ATCmd_SendResponse("Factory reset successful\r\n");
        return ATCMD_OK;
    }
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleDevEUI(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* Get DevEUI */
        char hexStr[17];
        ATCmd_BytesToHexString(storage.DevEui, 8, hexStr);
        ATCmd_SendFormattedResponse("+DEVEUI: %s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* Set DevEUI */
        if (strlen(argv[1]) != 16)
        {
            return ATCMD_INVALID_PARAM;
        }
        if (ATCmd_HexStringToBytes(argv[1], storage.DevEui, 8))
        {
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleAppEUI(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        char hexStr[17];
        ATCmd_BytesToHexString(storage.AppEui, 8, hexStr);
        ATCmd_SendFormattedResponse("+APPEUI: %s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        if (strlen(argv[1]) != 16)
        {
            return ATCMD_INVALID_PARAM;
        }
        if (ATCmd_HexStringToBytes(argv[1], storage.AppEui, 8))
        {
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleAppKey(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        char hexStr[33];
        ATCmd_BytesToHexString(storage.AppKey, 16, hexStr);
        ATCmd_SendFormattedResponse("+APPKEY: %s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        if (strlen(argv[1]) != 32)
        {
            return ATCMD_INVALID_PARAM;
        }
        if (ATCmd_HexStringToBytes(argv[1], storage.AppKey, 16))
        {
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleJoin(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    /* Check join mode */
    if (storage.JoinMode == 0)  /* ABP mode */
    {
        /* ABP mode - activate session without sending Join Request */
        if (storage.DevAddr == 0)
        {
            ATCmd_SendResponse("ABP mode: DevAddr not configured\r\n");
            return ATCMD_ERROR;
        }

        /* Session is already active in ABP mode (activated in LoRaWANApp_Init) */
        ATCmd_SendResponse(ATCMD_RESP_OK);
        ATCmd_SendResponse(ATCMD_RESP_JOINED);  /* "ABC JOINED" */
        return ATCMD_OK;
    }
    else  /* OTAA mode */
    {
        /* OTAA mode - send Join Request */
        if (LoRaWANApp_Join())
        {
            ATCmd_SendResponse(ATCMD_RESP_OK);
            return ATCMD_OK;
        }
        ATCmd_SendResponse(ATCMD_RESP_ERROR);
        return ATCMD_ERROR;
    }
}

static ATCmdResult_t ATCmd_HandleADR(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+ADR: %d\r\n", storage.AdrEnabled);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value == 0 || value == 1)
        {
            storage.AdrEnabled = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleDataRate(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+DR: %d\r\n", storage.DataRate);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value >= 0 && value <= 5)
        {
            storage.DataRate = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleTxPower(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+TXP: %d\r\n", storage.TxPower);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value >= 0 && value <= 10)
        {
            storage.TxPower = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleTDC(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("AT+TDC=%u\r\n", storage.TxDutyCycle);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);

        /* Validate minimum TDC (from original firmware) */
        if (value < TDC_MINIMUM_MS)
        {
            ATCmd_SendResponse(TDC_ERROR_MSG);
            return ATCMD_ERROR;
        }

        storage.TxDutyCycle = value;
        Storage_Save(&storage);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandlePort(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+PORT: %d\r\n", storage.AppPort);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value >= 1 && value <= 223)
        {
            storage.AppPort = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleConfirmed(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+PNACKMD: %d\r\n", storage.ConfirmedMsg);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value == 0 || value == 1)
        {
            storage.ConfirmedMsg = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleRX2DR(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+RX2DR: %d\r\n", storage.Rx2DataRate);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        storage.Rx2DataRate = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleRX2FQ(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+RX2FQ: %lu\r\n", storage.Rx2Frequency);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        uint32_t value = atoi(argv[1]);
        storage.Rx2Frequency = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleFreqBand(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+FREQBAND: %d\r\n", storage.FreqBand);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        int value = atoi(argv[1]);
        if (value >= 1 && value <= 9)
        {
            storage.FreqBand = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleBattery(int argc, char *argv[])
{
    uint8_t battLevel = BoardGetBatteryLevel();
    float battVoltage = (battLevel * 3.3) / 254.0;
    ATCmd_SendFormattedResponse("+BAT: %.2fV (%d%%)\r\n", battVoltage, (battLevel * 100) / 254);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleDebug(int argc, char *argv[])
{
    if (argc == 1)
    {
        ATCmd_SendFormattedResponse("+DEBUG: %d\r\n", DEBUG_ENABLED);
        return ATCMD_OK;
    }
    /* Debug enable/disable would require recompile */
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleCalibRemote(int argc, char *argv[])
{
    if (argc >= 2)
    {
        /* Parse hex payload and send to calibration module */
        uint8_t payload[32];
        uint8_t len = (strlen(argv[1]) + 1) / 2;

        if (len > 32) len = 32;

        if (ATCmd_HexStringToBytes(argv[1], payload, len))
        {
            if (Calibration_ProcessATCommand(payload, len))
            {
                ATCmd_SendResponse("Calibration command processed\r\n");
                return ATCMD_OK;
            }
        }
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ========================================================================== */

static void ATCmd_ParseCommand(char *cmdLine, int *argc, char *argv[])
{
    *argc = 0;
    char *token = strtok(cmdLine, "=, ");

    while (token != NULL && *argc < ATCMD_MAX_ARGS)
    {
        argv[(*argc)++] = token;
        token = strtok(NULL, "=, ");
    }
}

static uint8_t ATCmd_HexCharToNibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static bool ATCmd_HexStringToBytes(const char *hexStr, uint8_t *bytes, uint8_t len)
{
    if (strlen(hexStr) != len * 2)
    {
        return false;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        bytes[i] = (ATCmd_HexCharToNibble(hexStr[i * 2]) << 4) |
                    ATCmd_HexCharToNibble(hexStr[i * 2 + 1]);
    }

    return true;
}

static void ATCmd_BytesToHexString(const uint8_t *bytes, uint8_t len, char *hexStr)
{
    const char hexChars[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < len; i++)
    {
        hexStr[i * 2] = hexChars[(bytes[i] >> 4) & 0x0F];
        hexStr[i * 2 + 1] = hexChars[bytes[i] & 0x0F];
    }
    hexStr[len * 2] = '\0';
}

/* ============================================================================
 * ABP MODE HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleNetworkJoinMode(int argc, char *argv[])
{
    StorageData_t storageData;
    Storage_Load(&storageData);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("AT+NJM=%d\r\n", storageData.JoinMode);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int mode = atoi(argv[1]);
        if (mode != 0 && mode != 1)
        {
            ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
            return ATCMD_ERROR;
        }
        storageData.JoinMode = (uint8_t)mode;
        Storage_Save(&storageData);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleNwkSKey(int argc, char *argv[])
{
    StorageData_t storageData;
    Storage_Load(&storageData);

    if (argc == 1)
    {
        /* GET */
        char hexStr[33];
        ATCmd_BytesToHexString(storageData.NwkSKey, 16, hexStr);
        ATCmd_SendFormattedResponse("AT+NWKSKEY=%s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        if (!ATCmd_HexStringToBytes(argv[1], storageData.NwkSKey, 16))
        {
            ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
            return ATCMD_ERROR;
        }
        Storage_Save(&storageData);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleAppSKey(int argc, char *argv[])
{
    StorageData_t storageData;
    Storage_Load(&storageData);

    if (argc == 1)
    {
        /* GET */
        char hexStr[33];
        ATCmd_BytesToHexString(storageData.AppSKey, 16, hexStr);
        ATCmd_SendFormattedResponse("AT+APPSKEY=%s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        if (!ATCmd_HexStringToBytes(argv[1], storageData.AppSKey, 16))
        {
            ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
            return ATCMD_ERROR;
        }
        Storage_Save(&storageData);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleDevAddr(int argc, char *argv[])
{
    StorageData_t storageData;
    Storage_Load(&storageData);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("AT+DADDR=%08X\r\n", storageData.DevAddr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint8_t addrBytes[4];
        if (!ATCmd_HexStringToBytes(argv[1], addrBytes, 4))
        {
            ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
            return ATCMD_ERROR;
        }
        storageData.DevAddr = ((uint32_t)addrBytes[0] << 24) |
                              ((uint32_t)addrBytes[1] << 16) |
                              ((uint32_t)addrBytes[2] << 8) |
                              ((uint32_t)addrBytes[3]);
        Storage_Save(&storageData);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleDisableFrameCounterCheck(int argc, char *argv[])
{
    StorageData_t storageData;
    Storage_Load(&storageData);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("AT+DISFCNTCHECK=%d\r\n", storageData.DisableFrameCounterCheck);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int value = atoi(argv[1]);
        if (value != 0 && value != 1)
        {
            ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
            return ATCMD_ERROR;
        }
        storageData.DisableFrameCounterCheck = (uint8_t)value;
        Storage_Save(&storageData);
        ATCmd_SendResponse(ATCMD_RESP_OK);
        return ATCMD_OK;
    }

    ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleNetworkJoinStatus(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        ATCmd_SendResponse(ATCMD_RESP_PARAM_ERROR);
        return ATCMD_ERROR;
    }

    /* Query LoRaWAN stack for join status */
    bool isJoined = LoRaWANApp_IsJoined();
    ATCmd_SendFormattedResponse("AT+NJS=%d\r\n", isJoined ? 1 : 0);
    return ATCMD_OK;
}

/* ============================================================================
 * RX WINDOW CONFIGURATION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleRX1DL(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+RX1DL: %lu\r\n", storage.Rx1Delay);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);
        storage.Rx1Delay = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleRX2DL(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+RX2DL: %lu\r\n", storage.Rx2Delay);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);
        storage.Rx2Delay = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleJRX1DL(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+JRX1DL: %lu\r\n", storage.JoinRx1Delay);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);
        storage.JoinRx1Delay = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleJRX2DL(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+JRX2DL: %lu\r\n", storage.JoinRx2Delay);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);
        storage.JoinRx2Delay = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * FRAME COUNTER HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleFrameCounterUp(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+FCU: %lu\r\n", storage.FrameCounterUp);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleFrameCounterDown(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+FCD: %lu\r\n", storage.FrameCounterDown);
    return ATCMD_OK;
}

/* ============================================================================
 * SIGNAL QUALITY HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleRSSI(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+RSSI: %d\r\n", g_LastRSSI);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleSNR(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+SNR: %d\r\n", g_LastSNR);
    return ATCMD_OK;
}

/* ============================================================================
 * DEVICE CONFIGURATION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleClass(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET - convert numeric to letter */
        char classChar = 'A' + storage.DeviceClass;
        ATCmd_SendFormattedResponse("+CLASS: %c\r\n", classChar);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET - accept A, B, C or 0, 1, 2 */
        uint8_t classValue;
        if (argv[1][0] >= 'A' && argv[1][0] <= 'C')
        {
            classValue = argv[1][0] - 'A';
        }
        else if (argv[1][0] >= 'a' && argv[1][0] <= 'c')
        {
            classValue = argv[1][0] - 'a';
        }
        else if (argv[1][0] >= '0' && argv[1][0] <= '2')
        {
            classValue = argv[1][0] - '0';
        }
        else
        {
            return ATCMD_INVALID_PARAM;
        }

        storage.DeviceClass = classValue;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleRecv(int argc, char *argv[])
{
    /* Read-only command to check for pending downlink */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+RECV: %d\r\n", g_PendingDownlink);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleRetry(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+RETY: %d\r\n", storage.RetryCount);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int value = atoi(argv[1]);
        if (value >= 0 && value <= 15)  /* LoRaWAN spec allows up to 15 retries */
        {
            storage.RetryCount = (uint8_t)value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleRetryDelay(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+DELAY: %lu\r\n", storage.RetryDelay);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t value = (uint32_t)atoi(argv[1]);
        storage.RetryDelay = value;
        Storage_Save(&storage);
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * DATA TRANSMISSION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleSend(int argc, char *argv[])
{
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Check if joined */
    if (!LoRaWANApp_IsJoined())
    {
        ATCmd_SendResponse(ATCMD_RESP_NO_NET_JOINED);
        return ATCMD_ERROR;
    }

    /* Parse hex string to bytes */
    uint8_t payload[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
    uint8_t payloadLen = strlen(argv[1]) / 2;

    if (payloadLen > LORAWAN_APP_DATA_BUFFER_MAX_SIZE || strlen(argv[1]) % 2 != 0)
    {
        return ATCMD_INVALID_PARAM;
    }

    if (!ATCmd_HexStringToBytes(argv[1], payload, payloadLen))
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Get current settings */
    StorageData_t storage;
    Storage_Load(&storage);

    /* Send uplink */
    if (LoRaWANApp_SendUplink(payload, payloadLen, storage.AppPort, storage.ConfirmedMsg))
    {
        g_LastConfirmedStatus = 0;  /* Pending */
        return ATCMD_OK;
    }

    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleSendBinary(int argc, char *argv[])
{
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Check if joined */
    if (!LoRaWANApp_IsJoined())
    {
        ATCmd_SendResponse(ATCMD_RESP_NO_NET_JOINED);
        return ATCMD_ERROR;
    }

    /* Convert ASCII string to binary payload */
    uint8_t *payload = (uint8_t *)argv[1];
    uint8_t payloadLen = strlen(argv[1]);

    if (payloadLen > LORAWAN_APP_DATA_BUFFER_MAX_SIZE)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Get current settings */
    StorageData_t storage;
    Storage_Load(&storage);

    /* Send uplink */
    if (LoRaWANApp_SendUplink(payload, payloadLen, storage.AppPort, storage.ConfirmedMsg))
    {
        g_LastConfirmedStatus = 0;  /* Pending */
        return ATCMD_OK;
    }

    return ATCMD_ERROR;
}

static ATCmdResult_t ATCmd_HandleConfirmedMode(int argc, char *argv[])
{
    /* This is an alias for AT+PNACKMD */
    return ATCmd_HandleConfirmed(argc, argv);
}

static ATCmdResult_t ATCmd_HandleConfirmedStatus(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendFormattedResponse("+CFS: %d\r\n", g_LastConfirmedStatus);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleAppPort(int argc, char *argv[])
{
    /* This is an alias for AT+PORT */
    return ATCmd_HandlePort(argc, argv);
}

/* ============================================================================
 * TIME SYNCHRONIZATION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleTimeRequest(int argc, char *argv[])
{
    /* Read-only command to request time sync from network */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Check if joined */
    if (!LoRaWANApp_IsJoined())
    {
        ATCmd_SendResponse(ATCMD_RESP_NO_NET_JOINED);
        return ATCMD_ERROR;
    }

    /* Send DeviceTimeReq MAC command (would be implemented in LoRaWAN stack) */
    ATCmd_SendResponse("Time sync request sent\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleLocalTime(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Get local time from HAL (uptime in seconds) */
    uint32_t uptime = HAL_GetTick() / 1000;
    ATCmd_SendFormattedResponse("+LTIME: %lu\r\n", uptime);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleUTC(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+UTC: %lu\r\n", g_UTCTimeSeconds);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t utc = (uint32_t)atoi(argv[1]);
        g_UTCTimeSeconds = utc;
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * CHANNEL CONFIGURATION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleChannelEnable(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+CHE: 0x%04X\r\n", g_ChannelMask);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET - parse hex mask */
        uint16_t mask = (uint16_t)strtol(argv[1], NULL, 16);
        g_ChannelMask = mask;
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleChannelSingle(int argc, char *argv[])
{
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    int channel = atoi(argv[1]);
    if (channel < 0 || channel > 71)  /* AU915 has 72 channels (0-71) */
    {
        return ATCMD_INVALID_PARAM;
    }

    if (argc == 2)
    {
        /* GET - show if channel is enabled */
        uint8_t enabled = (g_ChannelMask & (1 << (channel % 16))) ? 1 : 0;
        ATCmd_SendFormattedResponse("+CHS: %d,%d\r\n", channel, enabled);
        return ATCMD_OK;
    }
    else if (argc == 3)
    {
        /* SET - enable/disable channel */
        int enable = atoi(argv[2]);
        if (enable == 1)
        {
            g_ChannelMask |= (1 << (channel % 16));
        }
        else if (enable == 0)
        {
            g_ChannelMask &= ~(1 << (channel % 16));
        }
        else
        {
            return ATCMD_INVALID_PARAM;
        }
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleChannel(int argc, char *argv[])
{
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    int channel = atoi(argv[1]);
    if (channel < 0 || channel > 71)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* For AU915, show channel frequency and DR range */
    /* Uplink channels: 915.2 + (channel * 0.2) MHz for ch 0-63 */
    /* Channel 64-71 are 125kHz channels for uplink */
    uint32_t freq;
    if (channel < 64)
    {
        freq = 915200000 + (channel * 200000);  /* 200 kHz spacing */
    }
    else
    {
        freq = 915900000 + ((channel - 64) * 1600000);  /* 1.6 MHz spacing */
    }

    ATCmd_SendFormattedResponse("+CH: %d,%lu,DR0-DR3\r\n", channel, freq);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleBandPlan(int argc, char *argv[])
{
    StorageData_t storage;
    Storage_Load(&storage);

    if (argc == 1)
    {
        /* GET - return current band plan (AU915) */
        ATCmd_SendFormattedResponse("+BAND: AU915,sub-band %d\r\n", storage.FreqBand);
        return ATCMD_OK;
    }
    /* SET not implemented - region is fixed */
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleChannelMask(int argc, char *argv[])
{
    /* This is an alias for AT+CHE */
    return ATCmd_HandleChannelEnable(argc, argv);
}

/* ============================================================================
 * SYSTEM MONITORING HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleTemperature(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Get temperature from MCU (STM32L0 has internal temp sensor) */
    /* Formula: Temp (°C) = (V25 - Vsense) / Avg_Slope + 25 */
    /* For now, return a placeholder value */
    int16_t temp = 25;  /* Would read from ADC in real implementation */

    ATCmd_SendFormattedResponse("+TEMP: %d\r\n", temp);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleVoltage(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Get VDD from internal reference (VREFINT) */
    /* Would use ADC to measure VREFINT and calculate VDD */
    uint16_t vdd_mv = 3300;  /* Placeholder: 3.3V */

    ATCmd_SendFormattedResponse("+VDD: %d\r\n", vdd_mv);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleChipID(int argc, char *argv[])
{
    /* Read-only command */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* STM32 Unique ID is 96 bits (12 bytes) at address 0x1FF80050 */
    uint32_t *uid = (uint32_t *)0x1FF80050;

    ATCmd_SendFormattedResponse("+ID: %08lX%08lX%08lX\r\n", uid[0], uid[1], uid[2]);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleSleep(int argc, char *argv[])
{
    if (argc < 2)
    {
        /* No parameter - enter sleep immediately */
        ATCmd_SendResponse("Entering sleep mode\r\n");
        HAL_Delay(100);  /* Allow UART to finish */
        /* Would call low-power sleep function here */
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* Sleep for specified duration (ms) */
        uint32_t duration = (uint32_t)atoi(argv[1]);
        ATCmd_SendFormattedResponse("Sleeping for %lu ms\r\n", duration);
        HAL_Delay(100);  /* Allow UART to finish */
        HAL_Delay(duration);  /* Sleep duration */
        ATCmd_SendResponse("Awake\r\n");
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleWakeup(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+WAKEUP: %lu\r\n", g_WakeupInterval);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        uint32_t interval = (uint32_t)atoi(argv[1]);
        g_WakeupInterval = interval;
        return ATCMD_OK;
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandlePlatformData(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET - return hex string */
        char hexStr[65];
        ATCmd_BytesToHexString(g_PlatformData, 32, hexStr);
        ATCmd_SendFormattedResponse("+PDTA: %s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET - parse hex string */
        uint8_t len = strlen(argv[1]) / 2;
        if (len > 32 || strlen(argv[1]) % 2 != 0)
        {
            return ATCMD_INVALID_PARAM;
        }
        if (ATCmd_HexStringToBytes(argv[1], g_PlatformData, len))
        {
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleUserSettings(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET - return hex string */
        char hexStr[65];
        ATCmd_BytesToHexString(g_UserSettings, 32, hexStr);
        ATCmd_SendFormattedResponse("+USET: %s\r\n", hexStr);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET - parse hex string */
        uint8_t len = strlen(argv[1]) / 2;
        if (len > 32 || strlen(argv[1]) % 2 != 0)
        {
            return ATCMD_INVALID_PARAM;
        }
        if (ATCmd_HexStringToBytes(argv[1], g_UserSettings, len))
        {
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * SENSOR INTEGRATION HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleSensorInit(int argc, char *argv[])
{
    /* Initialize sensor hardware */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    /* Power on sensor, configure UART/I2C, etc. */
    g_SensorPowerOn = 1;
    g_SensorInitialized = 1;

    ATCmd_SendResponse("Sensor initialized\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleSensorRead(int argc, char *argv[])
{
    /* Read sensor data */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    if (!g_SensorInitialized)
    {
        ATCmd_SendResponse("Sensor not initialized\r\n");
        return ATCMD_ERROR;
    }

    /* Read sensor data (placeholder values) */
    /* In real implementation, would communicate with sensor via UART/I2C */
    uint16_t value1 = 1234;  /* Example sensor reading */
    uint16_t value2 = 5678;  /* Example sensor reading */

    ATCmd_SendFormattedResponse("+SENSORREAD: %d,%d\r\n", value1, value2);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleSensorCalibrate(int argc, char *argv[])
{
    if (!g_SensorInitialized)
    {
        ATCmd_SendResponse("Sensor not initialized\r\n");
        return ATCMD_ERROR;
    }

    if (argc == 1)
    {
        /* Perform auto-calibration */
        ATCmd_SendResponse("Calibrating sensor...\r\n");
        HAL_Delay(1000);  /* Simulate calibration time */
        ATCmd_SendResponse("Sensor calibrated\r\n");
        return ATCMD_OK;
    }
    else if (argc >= 2)
    {
        /* Manual calibration with parameters */
        ATCmd_SendResponse("Manual calibration complete\r\n");
        return ATCMD_OK;
    }

    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleSensorMode(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+SENSORMODE: %d\r\n", g_SensorMode);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET - mode: 0=low power, 1=normal, 2=high precision */
        int mode = atoi(argv[1]);
        if (mode >= 0 && mode <= 2)
        {
            g_SensorMode = (uint8_t)mode;
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleSensorPower(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+SENSORPWR: %d\r\n", g_SensorPowerOn);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int power = atoi(argv[1]);
        if (power == 0)
        {
            /* Power off sensor */
            g_SensorPowerOn = 0;
            g_SensorInitialized = 0;
            ATCmd_SendResponse("Sensor powered off\r\n");
            return ATCMD_OK;
        }
        else if (power == 1)
        {
            /* Power on sensor */
            g_SensorPowerOn = 1;
            ATCmd_SendResponse("Sensor powered on\r\n");
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

/* ============================================================================
 * POWER MANAGEMENT & TESTING HANDLERS
 * ========================================================================== */

static ATCmdResult_t ATCmd_HandleLowPower(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+LOWPOWER: %d\r\n", g_LowPowerEnabled);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int enable = atoi(argv[1]);
        if (enable == 0 || enable == 1)
        {
            g_LowPowerEnabled = (uint8_t)enable;
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleDutyCycle(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+DUTYCYCLE: %d\r\n", g_DutyCycleEnabled);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int enable = atoi(argv[1]);
        if (enable == 0 || enable == 1)
        {
            g_DutyCycleEnabled = (uint8_t)enable;
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleLinkCheck(int argc, char *argv[])
{
    /* Request link check from network */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    if (!LoRaWANApp_IsJoined())
    {
        ATCmd_SendResponse(ATCMD_RESP_NO_NET_JOINED);
        return ATCMD_ERROR;
    }

    /* Send LinkCheckReq MAC command */
    ATCmd_SendResponse("Link check requested\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestTxCW(int argc, char *argv[])
{
    /* TX continuous wave for testing */
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    uint32_t freq = (uint32_t)atoi(argv[1]);
    uint8_t power = (argc >= 3) ? (uint8_t)atoi(argv[2]) : 14;

    ATCmd_SendFormattedResponse("TX CW: freq=%lu Hz, power=%d dBm\r\n", freq, power);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestConfig(int argc, char *argv[])
{
    /* Configure test mode parameters */
    ATCmd_SendResponse("Test configuration updated\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestOff(int argc, char *argv[])
{
    /* Disable test mode */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    g_TestModeEnabled = 0;
    ATCmd_SendResponse("Test mode disabled\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestOn(int argc, char *argv[])
{
    /* Enable test mode */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    g_TestModeEnabled = 1;
    ATCmd_SendResponse("Test mode enabled\r\n");
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestTone(int argc, char *argv[])
{
    /* Transmit tone for testing */
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    uint32_t freq = (uint32_t)atoi(argv[1]);
    uint16_t duration = (argc >= 3) ? (uint16_t)atoi(argv[2]) : 1000;

    ATCmd_SendFormattedResponse("TX Tone: freq=%lu Hz, duration=%d ms\r\n", freq, duration);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestRx(int argc, char *argv[])
{
    /* Test RX mode */
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    uint32_t freq = (uint32_t)atoi(argv[1]);
    ATCmd_SendFormattedResponse("RX Test mode: freq=%lu Hz\r\n", freq);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestTx(int argc, char *argv[])
{
    /* Test TX mode */
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    uint32_t freq = (uint32_t)atoi(argv[1]);
    uint8_t power = (argc >= 3) ? (uint8_t)atoi(argv[2]) : 14;

    ATCmd_SendFormattedResponse("TX Test mode: freq=%lu Hz, power=%d dBm\r\n", freq, power);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleTestRSSI(int argc, char *argv[])
{
    /* Test RSSI measurement */
    if (argc < 2)
    {
        return ATCMD_INVALID_PARAM;
    }

    uint32_t freq = (uint32_t)atoi(argv[1]);
    int16_t rssi = -80;  /* Placeholder value */

    ATCmd_SendFormattedResponse("+TRSSI: freq=%lu Hz, RSSI=%d dBm\r\n", freq, rssi);
    return ATCMD_OK;
}

static ATCmdResult_t ATCmd_HandleLog(int argc, char *argv[])
{
    if (argc == 1)
    {
        /* GET */
        ATCmd_SendFormattedResponse("+LOG: %d\r\n", g_LoggingEnabled);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        /* SET */
        int enable = atoi(argv[1]);
        if (enable == 0 || enable == 1)
        {
            g_LoggingEnabled = (uint8_t)enable;
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
}

static ATCmdResult_t ATCmd_HandleHelp(int argc, char *argv[])
{
    /* List all available commands */
    if (argc != 1)
    {
        return ATCMD_INVALID_PARAM;
    }

    ATCmd_SendResponse("\r\n=== Available AT Commands ===\r\n\r\n");

    for (uint32_t i = 0; i < ATCMD_TABLE_SIZE; i++)
    {
        ATCmd_SendFormattedResponse("%-20s - %s\r\n",
            g_ATCmdTable[i].name,
            g_ATCmdTable[i].help);
    }

    ATCmd_SendFormattedResponse("\r\nTotal commands: %lu\r\n\r\n", ATCMD_TABLE_SIZE);
    return ATCMD_OK;
}
