/*!
 * \file      power.c
 *
 * \brief     Power management implementation
 *
 * \details   Implements ultra-low power modes for STM32L072CZ.
 *            Targets <20µA in STOP mode with RTC wake-up.
 */
#include <stdio.h>
#include "power.h"
#include "config.h"
#include "board-config.h"
#include "board.h"
#include "radio.h"
#include "rtc-board.h"
#include "lpm-board.h"
#include "watchdog.h"
#include "utilities.h"
#include "stm32l0xx.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static WakeupSource_t g_WakeupSource = WAKEUP_SOURCE_NONE;
static bool g_PowerInitialized = false;
static bool g_RtcWakeScheduled = false;
static uint32_t g_WakeupTimeoutTicks = 0;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static void Power_EnableRtcClock(void);
static void Power_ArmWakeupTimer(uint32_t wakeupTimeMs);
static void Power_DisarmWakeupTimer(void);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool Power_Init(void)
{
    if (g_PowerInitialized)
    {
        return true;
    }

    /* Enable RTC domain and tick driver */
    Power_EnableRtcClock();
    RtcInit();

    g_PowerInitialized = true;
    DEBUG_PRINT("Power management initialized\r\n");

    return true;
}

WakeupSource_t Power_EnterStopMode(uint32_t wakeupTimeMs)
{
    if (!g_PowerInitialized)
    {
        return WAKEUP_SOURCE_NONE;
    }

    DEBUG_PRINT("Entering STOP mode for %u ms\r\n", (unsigned int)wakeupTimeMs);

    /* Reset wake-up bookkeeping */
    g_WakeupSource = WAKEUP_SOURCE_NONE;

    #if WATCHDOG_ENABLED
    /* The IWDG continues running in STOP mode!
     * If requested sleep time exceeds watchdog safe time,
     * we must wake up periodically to refresh the watchdog.
     */
    uint32_t remainingTimeMs = wakeupTimeMs;
    uint32_t maxStopTimeMs = Watchdog_GetMaxStopTime();

    while (remainingTimeMs > 0U)
    {
        uint32_t thisSleeepMs = (remainingTimeMs > maxStopTimeMs) ? maxStopTimeMs : remainingTimeMs;

        /* Refresh watchdog before entering STOP */
        Watchdog_Refresh();

        /* Disable peripherals to save power */
        Power_DisablePeripherals();

        /* Put radio in sleep mode (only on first iteration) */
        if (remainingTimeMs == wakeupTimeMs)
        {
            Power_RadioSleep();
        }

        /* Program wake-up timer */
        Power_ArmWakeupTimer(thisSleeepMs);

        /* Enter STOP mode */
        LpmEnterStopMode();

        /* Restore clocks after STOP */
        LpmExitStopMode();

        /* Refresh watchdog immediately after wake-up */
        Watchdog_Refresh();

        /* Evaluate wake-up cause */
        if (g_RtcWakeScheduled)
        {
            uint32_t elapsed = RtcGetTimerElapsedTime();
            if (elapsed >= g_WakeupTimeoutTicks)
            {
                g_WakeupSource = WAKEUP_SOURCE_RTC;
            }
        }

        /* Clean up timer state */
        Power_DisarmWakeupTimer();

        /* Update remaining time */
        remainingTimeMs = (remainingTimeMs > thisSleeepMs) ? (remainingTimeMs - thisSleeepMs) : 0U;

        /* Re-enable peripherals for next iteration or final wake-up */
        Power_EnablePeripherals();
    }

    /* Wake radio only at final wake-up */
    Power_RadioWakeup();

    #else
    /* Watchdog not enabled, use original single-shot sleep */

    /* Disable peripherals to save power */
    Power_DisablePeripherals();

    /* Put radio in sleep mode */
    Power_RadioSleep();

    /* Program wake-up timer if requested */
    Power_ArmWakeupTimer(wakeupTimeMs);

    /* Enter STOP mode */
    LpmEnterStopMode();

    /* Restore clocks after STOP */
    LpmExitStopMode();

    /* Evaluate wake-up cause */
    if (g_RtcWakeScheduled)
    {
        uint32_t elapsed = RtcGetTimerElapsedTime();
        if (elapsed >= g_WakeupTimeoutTicks)
        {
            g_WakeupSource = WAKEUP_SOURCE_RTC;
        }
    }

    /* Clean up timer state */
    Power_DisarmWakeupTimer();

    /* Re-enable peripherals */
    Power_EnablePeripherals();
    Power_RadioWakeup();
    #endif

    DEBUG_PRINT("Woke up from STOP mode (source: %d)\r\n", g_WakeupSource);

    return g_WakeupSource;
}

void Power_EnterSleepMode(void)
{
    LpmEnterSleepMode();
    LpmExitSleepMode();
}

void Power_ExitLowPowerMode(void)
{
    Power_EnablePeripherals();
    Power_RadioWakeup();
}

void Power_DisablePeripherals(void)
{
    /* TODO: Gate peripheral clocks that are not required during STOP. */
}

void Power_EnablePeripherals(void)
{
    /* TODO: Re-enable any clocks disabled in Power_DisablePeripherals. */
}

void Power_RadioSleep(void)
{
    Radio.Sleep();
}

void Power_RadioWakeup(void)
{
    Radio.Standby();
}

WakeupSource_t Power_GetWakeupSource(void)
{
    return g_WakeupSource;
}

uint32_t Power_MeasureCurrentConsumption(void)
{
    /* This is a stub function for debugging */
    /* In production, you would measure actual current using ADC */
    /* For now, return estimated value based on power mode */

    if ((RCC->CSR & RCC_CSR_LSERDY) != 0U)
    {
        /* LSE running implies STOP mode with RTC: estimate ~10µA */
        return 10;
    }

    /* In RUN mode: estimate ~1mA */
    return 1000;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static void Power_EnableRtcClock(void)
{
    /* Enable PWR clock and allow access to backup domain */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;

    /* Enable LSE as RTC clock source */
    if ((RCC->CSR & RCC_CSR_LSERDY) == 0U)
    {
        RCC->CSR |= RCC_CSR_LSEON;
        while ((RCC->CSR & RCC_CSR_LSERDY) == 0U)
        {
        }
    }

    /* Select LSE and enable RTC */
    RCC->CSR &= ~RCC_CSR_RTCSEL;
    RCC->CSR |= RCC_CSR_RTCSEL_LSE;
    RCC->CSR |= RCC_CSR_RTCEN;

    /* Disable backup domain access */
    PWR->CR &= ~PWR_CR_DBP;
}

static void Power_ArmWakeupTimer(uint32_t wakeupTimeMs)
{
    if (wakeupTimeMs == 0U)
    {
        g_RtcWakeScheduled = false;
        g_WakeupTimeoutTicks = 0U;
        return;
    }

    g_WakeupTimeoutTicks = RtcMs2Tick(wakeupTimeMs);
    RtcSetTimerContext();
    RtcStartAlarm(g_WakeupTimeoutTicks);
    g_RtcWakeScheduled = true;
}

static void Power_DisarmWakeupTimer(void)
{
    if (g_RtcWakeScheduled)
    {
        RtcStopAlarm();
    }
    g_RtcWakeScheduled = false;
    g_WakeupTimeoutTicks = 0U;
}
