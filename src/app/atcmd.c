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

    /* System commands */
    { "AT+BAT", ATCmd_HandleBattery, "Get battery level" },
    { "AT+DEBUG", ATCmd_HandleDebug, "Get/Set debug mode (0/1)" },

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
    if (LoRaWANApp_Join())
    {
        ATCmd_SendResponse("Joining network...\r\n");
        return ATCMD_OK;
    }
    return ATCMD_ERROR;
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
        ATCmd_SendFormattedResponse("+TDC: %lu\r\n", storage.TxDutyCycle);
        return ATCMD_OK;
    }
    else if (argc == 2)
    {
        uint32_t value = atoi(argv[1]);
        if (value >= 1000)  /* Minimum 1 second */
        {
            storage.TxDutyCycle = value;
            Storage_Save(&storage);
            return ATCMD_OK;
        }
    }
    return ATCMD_INVALID_PARAM;
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
