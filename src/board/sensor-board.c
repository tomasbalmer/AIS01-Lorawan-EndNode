#include "sensor-board.h"

#include <string.h>

#include "board-config.h"
#include "config.h"
#include "delay.h"
#include "fifo.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

#ifndef SENSOR_UART_ID
#define SENSOR_UART_ID UART_1
#endif

#ifndef SENSOR_UART_TX
#define SENSOR_UART_TX PA_9
#endif

#ifndef SENSOR_UART_RX
#define SENSOR_UART_RX PA_10
#endif

#ifndef SENSOR_UART_CMD_TIMEOUT_MS
#define SENSOR_UART_CMD_TIMEOUT_MS 200U
#endif

#ifndef SENSOR_UART_INTERBYTE_TIMEOUT_MS
#define SENSOR_UART_INTERBYTE_TIMEOUT_MS 20U
#endif

#ifndef SENSOR_FRAME_MAX_SIZE
#define SENSOR_FRAME_MAX_SIZE 64U
#endif

#define SENSOR_UART_TX_FIFO_SIZE 128U
#define SENSOR_UART_RX_FIFO_SIZE 128U

#define AIS01_CMD_STATUS   0x00U
#define AIS01_CMD_CAPTURE  0x01U

typedef struct
{
    bool configured;
    bool powered;
    bool uartReady;
    bool powerPinInit;
    Uart_t uart;
    uint8_t txFifo[SENSOR_UART_TX_FIFO_SIZE];
    uint8_t rxFifo[SENSOR_UART_RX_FIFO_SIZE];
    Gpio_t powerPin;
    uint8_t frameBuffer[SENSOR_FRAME_MAX_SIZE];
    uint16_t frameLength;
    bool frameReady;
    uint8_t scratchBuffer[SENSOR_FRAME_MAX_SIZE];
    uint16_t scratchLength;
    TimerTime_t lastRxTick;
} SensorBridgeContext_t;

static SensorBridgeContext_t g_SensorBridgeCtx = { 0 };

static void SensorBridge_EnablePower(bool enable);
static void SensorBridge_ClearBuffers(void);
static void SensorBridge_ProcessRx(void);
static bool SensorBridge_SendCommand(uint8_t opcode, uint32_t parameter);
static void SensorBridge_StoreFrame(const uint8_t *data, uint16_t len);
static bool SensorBridge_ReadBlocking(uint32_t timeoutMs, uint8_t *buffer, uint16_t *len);
static bool SensorBridge_ParseMeasurement(const uint8_t *buffer, uint16_t len, uint16_t *primary, uint16_t *secondary);

bool Sensor_BoardConfigure(void)
{
    if (g_SensorBridgeCtx.configured)
    {
        return true;
    }

    memset(&g_SensorBridgeCtx, 0, sizeof(g_SensorBridgeCtx));

    if (SENSOR_POWER_ENABLE != NC)
    {
        GpioInit(&g_SensorBridgeCtx.powerPin, SENSOR_POWER_ENABLE, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
        g_SensorBridgeCtx.powerPinInit = true;
    }

    FifoInit(&g_SensorBridgeCtx.uart.FifoTx, g_SensorBridgeCtx.txFifo, sizeof(g_SensorBridgeCtx.txFifo));
    FifoInit(&g_SensorBridgeCtx.uart.FifoRx, g_SensorBridgeCtx.rxFifo, sizeof(g_SensorBridgeCtx.rxFifo));
    UartInit(&g_SensorBridgeCtx.uart, SENSOR_UART_ID, SENSOR_UART_TX, SENSOR_UART_RX);
    UartConfig(&g_SensorBridgeCtx.uart, RX_TX, SENSOR_UART_BAUDRATE, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL);
    g_SensorBridgeCtx.uartReady = true;

    g_SensorBridgeCtx.configured = true;
    return true;
}

bool Sensor_BoardPowerControl(bool enable)
{
    if (!g_SensorBridgeCtx.configured && !Sensor_BoardConfigure())
    {
        return false;
    }

    if (enable)
    {
        if (g_SensorBridgeCtx.powered)
        {
            return true;
        }

        SensorBridge_EnablePower(true);
        if (SENSOR_POWER_ON_DELAY_MS > 0U)
        {
            DelayMs(SENSOR_POWER_ON_DELAY_MS);
        }
        SensorBridge_ClearBuffers();
        g_SensorBridgeCtx.powered = true;
        return true;
    }

    if (!g_SensorBridgeCtx.powered)
    {
        return true;
    }

    SensorBridge_EnablePower(false);
    SensorBridge_ClearBuffers();
    g_SensorBridgeCtx.powered = false;
    return true;
}

bool Sensor_BoardAcquire(uint16_t *primary, uint16_t *secondary)
{
    if (!Sensor_HwReady())
    {
        return false;
    }

    uint8_t frame[SENSOR_FRAME_MAX_SIZE] = { 0 };
    uint16_t frameLen = 0;

    SensorBridge_ClearBuffers();

    if (!SensorBridge_SendCommand(AIS01_CMD_CAPTURE, 0U))
    {
        return false;
    }

    if (!SensorBridge_ReadBlocking(SENSOR_UART_CMD_TIMEOUT_MS, frame, &frameLen))
    {
        return false;
    }

    SensorBridge_StoreFrame(frame, frameLen);
    return SensorBridge_ParseMeasurement(frame, frameLen, primary, secondary);
}

bool Sensor_HwFetchFrame(uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen)
{
    if ((buffer == NULL) || (maxLen == 0U) || !Sensor_HwReady())
    {
        return false;
    }

    SensorBridge_ProcessRx();

    if (!g_SensorBridgeCtx.frameReady)
    {
        return false;
    }

    uint16_t copyLen = (g_SensorBridgeCtx.frameLength > maxLen) ? maxLen : g_SensorBridgeCtx.frameLength;
    memcpy(buffer, g_SensorBridgeCtx.frameBuffer, copyLen);

    if (actualLen != NULL)
    {
        *actualLen = copyLen;
    }

    g_SensorBridgeCtx.frameReady = false;
    g_SensorBridgeCtx.frameLength = 0U;
    return true;
}

bool Sensor_HwPerformHandshake(void)
{
    if (!Sensor_HwReady())
    {
        return false;
    }

    SensorBridge_ClearBuffers();

    if (!SensorBridge_SendCommand(AIS01_CMD_STATUS, 0U))
    {
        return false;
    }

    uint8_t frame[SENSOR_FRAME_MAX_SIZE] = { 0 };
    uint16_t frameLen = 0;

    if (SensorBridge_ReadBlocking(SENSOR_UART_CMD_TIMEOUT_MS, frame, &frameLen))
    {
        SensorBridge_StoreFrame(frame, frameLen);
    }

    return true;
}

void Sensor_HwFlushRx(void)
{
    if (!g_SensorBridgeCtx.uartReady)
    {
        return;
    }

    uint8_t byte;
    while (UartGetChar(&g_SensorBridgeCtx.uart, &byte) == 0)
    {
        /* flush */
    }

    g_SensorBridgeCtx.frameReady = false;
    g_SensorBridgeCtx.frameLength = 0U;
    g_SensorBridgeCtx.scratchLength = 0U;
    g_SensorBridgeCtx.lastRxTick = 0U;
}

bool Sensor_HwReady(void)
{
    return g_SensorBridgeCtx.configured && g_SensorBridgeCtx.uartReady && g_SensorBridgeCtx.powered;
}

static void SensorBridge_EnablePower(bool enable)
{
    if (!g_SensorBridgeCtx.powerPinInit)
    {
        return;
    }

    if (enable)
    {
        GpioWrite(&g_SensorBridgeCtx.powerPin, 1);
    }
    else
    {
        GpioWrite(&g_SensorBridgeCtx.powerPin, 0);
    }
}

static void SensorBridge_ClearBuffers(void)
{
    g_SensorBridgeCtx.frameReady = false;
    g_SensorBridgeCtx.frameLength = 0U;
    g_SensorBridgeCtx.scratchLength = 0U;
    g_SensorBridgeCtx.lastRxTick = 0U;
    Sensor_HwFlushRx();
}

static void SensorBridge_ProcessRx(void)
{
    if (!g_SensorBridgeCtx.uartReady)
    {
        return;
    }

    uint8_t byte;
    bool received = false;
    while (UartGetChar(&g_SensorBridgeCtx.uart, &byte) == 0)
    {
        received = true;
        if (g_SensorBridgeCtx.scratchLength < SENSOR_FRAME_MAX_SIZE)
        {
            g_SensorBridgeCtx.scratchBuffer[g_SensorBridgeCtx.scratchLength++] = byte;
        }
        g_SensorBridgeCtx.lastRxTick = TimerGetCurrentTime();
    }

    if (!received && (g_SensorBridgeCtx.scratchLength > 0U))
    {
        if ((g_SensorBridgeCtx.lastRxTick != 0U) &&
            (TimerGetElapsedTime(g_SensorBridgeCtx.lastRxTick) >= SENSOR_UART_INTERBYTE_TIMEOUT_MS))
        {
            SensorBridge_StoreFrame(g_SensorBridgeCtx.scratchBuffer, g_SensorBridgeCtx.scratchLength);
            g_SensorBridgeCtx.scratchLength = 0U;
            g_SensorBridgeCtx.lastRxTick = 0U;
        }
    }
}

static bool SensorBridge_SendCommand(uint8_t opcode, uint32_t parameter)
{
    if (!g_SensorBridgeCtx.uartReady)
    {
        return false;
    }

    uint8_t frame[4];
    frame[0] = opcode;
    frame[1] = (uint8_t)(parameter & 0xFFU);
    frame[2] = (uint8_t)((parameter >> 8) & 0xFFU);
    frame[3] = (uint8_t)((parameter >> 16) & 0xFFU);

    return (UartPutBuffer(&g_SensorBridgeCtx.uart, frame, sizeof(frame)) == 0);
}

static void SensorBridge_StoreFrame(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U)
    {
        return;
    }

    uint16_t copyLen = (len > SENSOR_FRAME_MAX_SIZE) ? SENSOR_FRAME_MAX_SIZE : len;
    memcpy(g_SensorBridgeCtx.frameBuffer, data, copyLen);
    g_SensorBridgeCtx.frameLength = copyLen;
    g_SensorBridgeCtx.frameReady = true;
}

static bool SensorBridge_ReadBlocking(uint32_t timeoutMs, uint8_t *buffer, uint16_t *len)
{
    TimerTime_t start = TimerGetCurrentTime();

    while (TimerGetElapsedTime(start) < timeoutMs)
    {
        SensorBridge_ProcessRx();

        if (g_SensorBridgeCtx.frameReady)
        {
            uint16_t copyLen = g_SensorBridgeCtx.frameLength;
            if (copyLen > SENSOR_FRAME_MAX_SIZE)
            {
                copyLen = SENSOR_FRAME_MAX_SIZE;
            }

            if (buffer != NULL)
            {
                memcpy(buffer, g_SensorBridgeCtx.frameBuffer, copyLen);
            }

            if (len != NULL)
            {
                *len = copyLen;
            }

            g_SensorBridgeCtx.frameReady = false;
            g_SensorBridgeCtx.frameLength = 0U;
            return true;
        }

        DelayMs(1);
    }

    return false;
}

static bool SensorBridge_ParseMeasurement(const uint8_t *buffer, uint16_t len, uint16_t *primary, uint16_t *secondary)
{
    if ((buffer == NULL) || (len < 4U))
    {
        return false;
    }

    uint16_t primaryRaw = ((uint16_t)buffer[0] << 8U) | buffer[1];
    uint16_t secondaryRaw = ((uint16_t)buffer[2] << 8U) | buffer[3];

    if (primary != NULL)
    {
        *primary = primaryRaw;
    }

    if (secondary != NULL)
    {
        *secondary = secondaryRaw;
    }

    return true;
}
