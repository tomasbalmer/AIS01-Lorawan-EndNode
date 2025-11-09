#include <stdbool.h>
#include "stm32l072xx.h"
#include "stm32l0xx.h"
#include "pinName-board.h"
#include "utilities.h"
#include "gpio.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "adc-board.h"
#include "delay.h"
#include "rtc-board.h"
#include "sx1276-board.h"
#include "board-config.h"
#include "lpm-board.h"
#include "watchdog.h"
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
static Adc_t BatteryAdc;
static bool BatteryAdcInitialized = false;
static Gpio_t BatteryDividerPin;
static bool BatteryDividerConfigured = false;

static void BatteryDividerControl(bool enable);
static uint32_t BatteryCountsToMilliVolts(uint32_t counts);

static inline uint32_t BoardGetPrimask(void)
{
    uint32_t primask;
    __asm volatile("MRS %0, PRIMASK" : "=r"(primask));
    return primask;
}

static inline void BoardSetPrimask(uint32_t primask)
{
    __asm volatile("MSR PRIMASK, %0" : : "r"(primask) : "memory");
}

static inline void BoardDisableIrq(void)
{
    __asm volatile("cpsid i" : : : "memory");
}

static inline void BoardWfi(void)
{
    __asm volatile("wfi");
}

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
    *mask = BoardGetPrimask();
    BoardDisableIrq();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
    BoardSetPrimask(*mask);
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

        /* Initialize Independent Watchdog for system hang detection */
        /* WARNING: Once enabled, watchdog CANNOT be disabled! */
        #if WATCHDOG_ENABLED
        Watchdog_Init();
        #endif

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
    SCB->AIRCR = (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk);
    __asm volatile("dsb");
    while (1)
    {
        __asm volatile("nop");
    }
}

void BoardLowPowerHandler(void)
{
    BoardWfi();
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

static void BatteryDividerControl(bool enable)
{
    if (BATTERY_MEASURE_ENABLE == NC)
    {
        return;
    }

    if (!BatteryDividerConfigured)
    {
        GpioInit(&BatteryDividerPin, BATTERY_MEASURE_ENABLE, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
        BatteryDividerConfigured = true;
    }

    GpioWrite(&BatteryDividerPin, enable ? 1 : 0);
}

static uint32_t BatteryCountsToMilliVolts(uint32_t counts)
{
    if (BATTERY_DIVIDER_RLOWER_KOHM == 0U)
    {
        return 0U;
    }

    const uint32_t dividerFactor = BATTERY_DIVIDER_RUPPER_KOHM + BATTERY_DIVIDER_RLOWER_KOHM;
    uint64_t numerator = (uint64_t)counts * BATTERY_ADC_REFERENCE_MV * dividerFactor;
    numerator /= BATTERY_DIVIDER_RLOWER_KOHM;
    numerator /= BATTERY_ADC_FULL_SCALE;
    return (uint32_t)numerator;
}

uint16_t BoardBatteryMeasureVoltage(void)
{
    if (BATTERY_MEASURE_INPUT == NC)
    {
        return 0;
    }

    if (!BatteryAdcInitialized)
    {
        AdcMcuInit(&BatteryAdc, BATTERY_MEASURE_INPUT);
        BatteryAdcInitialized = true;
    }

    BatteryDividerControl(true);
    if (BATTERY_STABILIZATION_DELAY_MS > 0U)
    {
        DelayMs(BATTERY_STABILIZATION_DELAY_MS);
    }

    uint32_t sum = 0U;
    for (uint32_t i = 0U; i < BATTERY_SAMPLE_COUNT; i++)
    {
        sum += AdcMcuReadChannel(&BatteryAdc, BATTERY_ADC_CHANNEL);
    }

    BatteryDividerControl(false);

    if (BATTERY_SAMPLE_COUNT > 0U)
    {
        sum /= BATTERY_SAMPLE_COUNT;
    }

    uint32_t millivolts = BatteryCountsToMilliVolts(sum);
    if (millivolts > 0xFFFFU)
    {
        millivolts = 0xFFFFU;
    }

    return (uint16_t)millivolts;
}

uint32_t BoardGetBatteryVoltage(void)
{
    return (uint32_t)BoardBatteryMeasureVoltage();
}

uint8_t BoardGetBatteryLevel(void)
{
    const uint32_t mv = BoardGetBatteryVoltage();
    const uint32_t minMv = 3300U;
    const uint32_t maxMv = 4200U;

    if (mv == 0U)
    {
        return 0U;
    }

    if (mv >= maxMv)
    {
        return 254U;
    }

    if (mv <= minMv)
    {
        return 1U;
    }

    uint32_t scaled = ((mv - minMv) * 253U) / (maxMv - minMv);
    return (uint8_t)(scaled + 1U);
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
