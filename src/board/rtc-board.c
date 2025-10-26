#include <stdbool.h>
#include "stm32l0xx.h"
#include "rtc-board.h"

static volatile uint32_t RtcTick = 0;
static uint32_t RtcTimerContext = 0;
static volatile uint32_t AlarmTick = 0;
static volatile bool AlarmEnabled = false;
static bool RtcInitialized = false;
static uint32_t Backup0 = 0;
static uint32_t Backup1 = 0;

void RtcInit(void)
{
    if (!RtcInitialized)
    {
        SysTick_Config(SystemCoreClock / 1000U);
        RtcInitialized = true;
    }
}

uint32_t RtcSetTimerContext(void)
{
    RtcTimerContext = RtcGetTimerValue();
    return RtcTimerContext;
}

uint32_t RtcGetTimerContext(void)
{
    return RtcTimerContext;
}

uint32_t RtcGetMinimumTimeout(void)
{
    return 1U;
}

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
    return milliseconds;
}

uint32_t RtcTick2Ms(uint32_t tick)
{
    return tick;
}

void RtcDelayMs(uint32_t delay)
{
    uint32_t start = RtcGetTimerValue();
    while ((RtcGetTimerValue() - start) < delay)
    {
        __NOP();
    }
}

void RtcSetAlarm(uint32_t timeout)
{
    AlarmTick = RtcTimerContext + timeout;
    AlarmEnabled = true;
}

void RtcStopAlarm(void)
{
    AlarmEnabled = false;
}

void RtcStartAlarm(uint32_t timeout)
{
    RtcSetAlarm(timeout);
}

uint32_t RtcGetTimerValue(void)
{
    return RtcTick;
}

uint32_t RtcGetTimerElapsedTime(void)
{
    return RtcGetTimerValue() - RtcTimerContext;
}

uint32_t RtcGetCalendarTime(uint16_t *milliseconds)
{
    uint32_t ticks = RtcGetTimerValue();
    if (milliseconds != NULL)
    {
        *milliseconds = (uint16_t)(ticks % 1000U);
    }
    return ticks / 1000U;
}

void RtcBkupWrite(uint32_t data0, uint32_t data1)
{
    Backup0 = data0;
    Backup1 = data1;
}

void RtcBkupRead(uint32_t *data0, uint32_t *data1)
{
    if (data0 != NULL)
    {
        *data0 = Backup0;
    }
    if (data1 != NULL)
    {
        *data1 = Backup1;
    }
}

void RtcProcess(void)
{
}

TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature)
{
    (void)temperature;
    return period;
}

void RtcOnSysTick(void)
{
    RtcTick++;
    if (AlarmEnabled)
    {
        if ((int32_t)(RtcTick - AlarmTick) >= 0)
        {
            AlarmEnabled = false;
            TimerIrqHandler();
        }
    }
}
