#include <stdbool.h>
#include "stm32l072xx.h"
#include "stm32l0xx.h"
#include "pinName-board.h"
#include "utilities.h"
#include "gpio.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "rtc-board.h"
#include "sx1276-board.h"
#include "board-config.h"
#include "lpm-board.h"
#include "board.h"
#include "config.h"

/*!
 * Unique Devices IDs register set ( STM32L0xxx )
 */
#define ID1 (0x1FF80050)
#define ID2 (0x1FF80054)
#define ID3 (0x1FF80064)

static bool McuInitialized = false;
Uart_t Uart2;

static uint8_t Uart2TxBuffer[1024];
static uint8_t Uart2RxBuffer[1024];

static Gpio_t Led1;
static Gpio_t Led2;
static Gpio_t Led3;
static Gpio_t Led4;

static void SystemClockConfig(void)
{
    /* Enable HSI16 and use as system clock */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }

    SystemCoreClockUpdate();
}

static void BoardUnusedIoInit(void)
{
    (void)Led1;
}

void BoardCriticalSectionBegin(uint32_t *mask)
{
    *mask = __get_PRIMASK();
    __disable_irq();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
    __set_PRIMASK(*mask);
}

void BoardInitPeriph(void)
{
}

void BoardInitMcu(void)
{
    if (!McuInitialized)
    {
        SystemClockConfig();

        GpioInit(&Led1, LED_1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
        GpioInit(&Led2, LED_2, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
        GpioInit(&Led3, LED_3, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
        GpioInit(&Led4, LED_4, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);

        FifoInit(&Uart2.FifoTx, Uart2TxBuffer, sizeof(Uart2TxBuffer));
        FifoInit(&Uart2.FifoRx, Uart2RxBuffer, sizeof(Uart2RxBuffer));
        UartInit(&Uart2, UART_2, UART_TX, UART_RX);
        UartConfig(&Uart2, RX_TX, AT_UART_BAUDRATE, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL);

        RtcInit();
        BoardUnusedIoInit();
        LpmSetOffMode(LPM_APPLI_ID, LPM_DISABLE);

        McuInitialized = true;
    }

    SpiInit(&SX1276.Spi, SPI_1, RADIO_MOSI, RADIO_MISO, RADIO_SCLK, NC);
    SX1276IoInit();
    SX1276IoDbgInit();
    SX1276IoTcxoInit();
}

void BoardDeInitMcu(void)
{
    SpiDeInit(&SX1276.Spi);
    SX1276IoDeInit();
}

void BoardResetMcu(void)
{
    NVIC_SystemReset();
}

void BoardLowPowerHandler(void)
{
    __WFI();
}

uint32_t BoardGetRandomSeed(void)
{
    return (*(uint32_t *)ID1) ^ (*(uint32_t *)ID2) ^ (*(uint32_t *)ID3);
}

void BoardGetUniqueId(uint8_t *id)
{
    uint32_t id1 = *(uint32_t *)ID1;
    uint32_t id2 = *(uint32_t *)ID2;
    uint32_t id3 = *(uint32_t *)ID3;

    id[7] = (uint8_t)(id1 >> 24);
    id[6] = (uint8_t)(id1 >> 16);
    id[5] = (uint8_t)(id1 >> 8);
    id[4] = (uint8_t)(id1);
    id[3] = (uint8_t)(id2 >> 24);
    id[2] = (uint8_t)(id2 >> 16);
    id[1] = (uint8_t)(id2 >> 8);
    id[0] = (uint8_t)(id2);
    (void)id3;
}

uint16_t BoardBatteryMeasureVoltage(void)
{
    return 0;
}

uint32_t BoardGetBatteryVoltage(void)
{
    return 0;
}

uint8_t BoardGetBatteryLevel(void)
{
    return 0;
}

uint8_t GetBoardPowerSource(void)
{
    return BATTERY_POWER;
}

void LpmEnterSleepMode(void)
{
    __WFI();
}

void LpmExitSleepMode(void)
{
}

void LpmEnterStopMode(void)
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}

void LpmExitStopMode(void)
{
    SystemClockConfig();
}

void LpmEnterOffMode(void)
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR |= PWR_CR_PDDS;
    __WFI();
}

#include <stdio.h>

// Keil compiler
int fputc(int c, FILE *stream)
{
    while (UartPutChar(&Uart2, (uint8_t)c) != 0)
        ;
    return c;
}

int fgetc(FILE *stream)
{
    uint8_t c = 0;
    while (UartGetChar(&Uart2, &c) != 0)
        ;
    // Echo back the character
    while (UartPutChar(&Uart2, c) != 0)
        ;
    return (int)c;
}

void LpmExitOffMode(void)
{
    SystemClockConfig();
}
