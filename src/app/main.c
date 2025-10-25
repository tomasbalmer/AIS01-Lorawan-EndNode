/*!
 * \file      main.c
 *
 * \brief     Dragino AIS01-LB Main Application
 *
 * \details   State machine: BOOT -> JOIN -> IDLE -> UPLINK -> SLEEP -> WAKE -> IDLE
 *            Implements ultra-low power LoRaWAN Class A node for AU915.
 */
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "board.h"
#include "uart.h"
#include "timer.h"
#include "storage.h"
#include "calibration.h"
#include "power.h"
#include "atcmd.h"
#include "lorawan_app.h"

/* ============================================================================
 * APPLICATION STATE MACHINE
 * ========================================================================== */
typedef enum
{
    APP_STATE_BOOT = 0,               /* Initial boot and initialization */
    APP_STATE_JOIN,                   /* Joining LoRaWAN network */
    APP_STATE_IDLE,                   /* Idle state, waiting for events */
    APP_STATE_UPLINK,                 /* Sending uplink data */
    APP_STATE_RX,                     /* Receiving downlink (RX1/RX2) */
    APP_STATE_SLEEP,                  /* Enter low power mode */
} AppState_t;

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static AppState_t g_AppState = APP_STATE_BOOT;
static TimerEvent_t g_TxTimer;
static StorageData_t g_Config;
static volatile bool g_TxTimerExpired = false;

/* ============================================================================
 * EXTERNAL VARIABLES
 * ========================================================================== */
extern Uart_t Uart2;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static void OnTxTimerEvent(void *context);
static void PrepareUplinkPayload(uint8_t *buffer, uint8_t *size);
static void ProcessUartInput(void);

/* ============================================================================
 * MAIN FUNCTION
 * ========================================================================== */
int main(void)
{
    /* ========================================================================
     * BOOT STATE: Initialize all modules
     * ====================================================================== */
    g_AppState = APP_STATE_BOOT;

    /* Initialize MCU and peripherals */
    BoardInitMcu();
    BoardInitPeriph();

    DEBUG_PRINT("\r\n");
    DEBUG_PRINT("===================================\r\n");
    DEBUG_PRINT("  Dragino AIS01-LB Firmware\r\n");
    DEBUG_PRINT("  Version: %d.%d.%d\r\n", FIRMWARE_VERSION_MAJOR,
                FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    DEBUG_PRINT("  Region: AU915 Sub-band %d\r\n", LORAWAN_AU915_SUB_BAND);
    DEBUG_PRINT("===================================\r\n");

    /* Initialize storage */
    if (!Storage_Init())
    {
        DEBUG_PRINT("ERROR: Storage initialization failed\r\n");
        while (1);  /* Fatal error */
    }

    /* Load configuration */
    if (!Storage_Load(&g_Config))
    {
        DEBUG_PRINT("WARNING: No valid configuration, using defaults\r\n");
    }

    /* Initialize calibration module */
    if (!Calibration_Init())
    {
        DEBUG_PRINT("ERROR: Calibration initialization failed\r\n");
    }

    /* Initialize power management */
    if (!Power_Init())
    {
        DEBUG_PRINT("ERROR: Power management initialization failed\r\n");
    }

    /* Initialize AT command parser */
    if (!ATCmd_Init())
    {
        DEBUG_PRINT("ERROR: AT command parser initialization failed\r\n");
    }

    /* Initialize LoRaWAN stack */
    if (!LoRaWANApp_Init())
    {
        DEBUG_PRINT("ERROR: LoRaWAN initialization failed\r\n");
        while (1);  /* Fatal error */
    }

    /* Initialize TX timer */
    TimerInit(&g_TxTimer, OnTxTimerEvent);
    TimerSetValue(&g_TxTimer, g_Config.TxDutyCycle);

    DEBUG_PRINT("Initialization complete\r\n");

    /* ========================================================================
     * JOIN STATE: Join LoRaWAN network
     * ====================================================================== */
    g_AppState = APP_STATE_JOIN;

    /* Check if we have valid credentials */
    bool hasValidCredentials = false;
    for (int i = 0; i < 8; i++)
    {
        if (g_Config.DevEui[i] != 0x00 || g_Config.AppEui[i] != 0x00)
        {
            hasValidCredentials = true;
            break;
        }
    }

    if (hasValidCredentials)
    {
        DEBUG_PRINT("Starting OTAA join...\r\n");
        LoRaWANApp_Join();
    }
    else
    {
        DEBUG_PRINT("WARNING: No LoRaWAN credentials configured\r\n");
        DEBUG_PRINT("Please configure using AT commands:\r\n");
        DEBUG_PRINT("  AT+DEVEUI=<16-char hex>\r\n");
        DEBUG_PRINT("  AT+APPEUI=<16-char hex>\r\n");
        DEBUG_PRINT("  AT+APPKEY=<32-char hex>\r\n");
        DEBUG_PRINT("  AT+JOIN\r\n");
        g_AppState = APP_STATE_IDLE;
    }

    /* ========================================================================
     * MAIN LOOP: State machine
     * ====================================================================== */
    while (1)
    {
        /* Process UART input for AT commands */
        ProcessUartInput();

        /* Process LoRaWAN events */
        LoRaWANApp_Process();

        /* State machine */
        switch (g_AppState)
        {
            case APP_STATE_BOOT:
                /* Already handled above */
                break;

            case APP_STATE_JOIN:
            {
                /* Wait for join to complete */
                LoRaWANStatus_t status = LoRaWANApp_GetStatus();

                if (status == LORAWAN_STATUS_JOINED)
                {
                    DEBUG_PRINT("Network joined successfully\r\n");
                    g_AppState = APP_STATE_IDLE;

                    /* Start periodic uplink timer */
                    TimerStart(&g_TxTimer);
                }
                else if (status == LORAWAN_STATUS_JOIN_FAILED)
                {
                    /* Join will retry automatically */
                    DEBUG_PRINT("Join failed, retrying...\r\n");
                }
                break;
            }

            case APP_STATE_IDLE:
            {
                /* Check if TX timer expired */
                if (g_TxTimerExpired && LoRaWANApp_IsJoined())
                {
                    g_TxTimerExpired = false;
                    g_AppState = APP_STATE_UPLINK;
                }
                else if (!LoRaWANApp_IsJoined())
                {
                    /* Lost connection, rejoin */
                    g_AppState = APP_STATE_JOIN;
                    LoRaWANApp_Join();
                }
                else
                {
                    /* Enter low power mode until next event */
#if LOW_POWER_MODE_ENABLED
                    g_AppState = APP_STATE_SLEEP;
#endif
                }
                break;
            }

            case APP_STATE_UPLINK:
            {
                /* Prepare and send uplink payload */
                uint8_t buffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
                uint8_t size = 0;

                PrepareUplinkPayload(buffer, &size);

                bool confirmed = (g_Config.ConfirmedMsg != 0);

                if (LoRaWANApp_SendUplink(buffer, size, g_Config.AppPort, confirmed))
                {
                    DEBUG_PRINT("Uplink sent (%d bytes)\r\n", size);
                }
                else
                {
                    DEBUG_PRINT("Uplink failed\r\n");
                }

                /* Restart TX timer */
                TimerStart(&g_TxTimer);

                g_AppState = APP_STATE_IDLE;
                break;
            }

            case APP_STATE_RX:
            {
                /* Downlink reception is handled by callbacks */
                g_AppState = APP_STATE_IDLE;
                break;
            }

            case APP_STATE_SLEEP:
            {
#if LOW_POWER_MODE_ENABLED
                /* Calculate time until next event */
                uint32_t sleepTime = g_Config.TxDutyCycle;

                /* Enter STOP mode with RTC wake-up */
                Power_EnterStopMode(sleepTime);

                /* After wake-up, return to IDLE */
                g_AppState = APP_STATE_IDLE;
#else
                g_AppState = APP_STATE_IDLE;
#endif
                break;
            }

            default:
                g_AppState = APP_STATE_IDLE;
                break;
        }
    }

    return 0;  /* Never reached */
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static void OnTxTimerEvent(void *context)
{
    TimerStop(&g_TxTimer);
    g_TxTimerExpired = true;
}

static void PrepareUplinkPayload(uint8_t *buffer, uint8_t *size)
{
    /* Example payload: Battery level + sensor data (stub) */
    uint8_t index = 0;

    /* Byte 0: Battery level (0-254) */
    buffer[index++] = BoardGetBatteryLevel();

    /* Bytes 1-2: Temperature (placeholder) */
    int16_t temperature = 2200;  /* 22.0 °C */
    buffer[index++] = (temperature >> 8) & 0xFF;
    buffer[index++] = temperature & 0xFF;

    /* Bytes 3-4: Sensor value (placeholder) */
    uint16_t sensorValue = 1234;
    buffer[index++] = (sensorValue >> 8) & 0xFF;
    buffer[index++] = sensorValue & 0xFF;

    *size = index;
}

static void ProcessUartInput(void)
{
    /* Check for UART data */
    uint8_t rxChar;
    if (UartGetChar(&Uart2, &rxChar) == 0)
    {
        /* Process character through AT command parser */
        ATCmd_ProcessChar(rxChar);
    }
}

/* ============================================================================
 * PRINTF REDIRECTION (for DEBUG_PRINT)
 * ========================================================================== */
#ifdef __GNUC__
int _write(int fd, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        UartPutChar(&Uart2, ptr[i]);
    }
    return len;
}
#endif
