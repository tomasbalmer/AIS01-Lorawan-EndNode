/*!
 * \file      power.c
 *
 * \brief     Power management implementation
 *
 * \details   Implements ultra-low power modes for STM32L072CZ.
 *            Targets <20µA in STOP mode with RTC wake-up.
 */
#include "power.h"
#include "config.h"
#include "board-config.h"
#include "stm32l0xx_hal.h"
#include "board.h"
#include "radio.h"

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */
static WakeupSource_t g_WakeupSource = WAKEUP_SOURCE_NONE;
static bool g_PowerInitialized = false;

/* ============================================================================
 * EXTERNAL VARIABLES
 * ========================================================================== */
extern RTC_HandleTypeDef RtcHandle;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ========================================================================== */
static void Power_ConfigureRTCWakeup(uint32_t wakeupTimeMs);
static void Power_SystemClockConfig_MSI(void);

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool Power_Init(void)
{
    if (g_PowerInitialized)
    {
        return true;
    }

    /* Configure RTC for wake-up */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Enable wake-up from STOP mode via RTC */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

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

    DEBUG_PRINT("Entering STOP mode for %lu ms\r\n", wakeupTimeMs);

    /* Disable peripherals to save power */
    Power_DisablePeripherals();

    /* Put radio in sleep mode */
    Power_RadioSleep();

    /* Configure RTC wake-up timer */
    if (wakeupTimeMs > 0)
    {
        Power_ConfigureRTCWakeup(wakeupTimeMs);
    }

    /* Clear wake-up flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Enter STOP mode with regulator in low power mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* After wake-up, system clock is MSI, reconfigure to HSI if needed */
    Power_SystemClockConfig_MSI();

    /* Determine wake-up source */
    if (__HAL_RTC_WAKEUPTIMER_GET_FLAG(&RtcHandle, RTC_FLAG_WUTF))
    {
        g_WakeupSource = WAKEUP_SOURCE_RTC;
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&RtcHandle, RTC_FLAG_WUTF);
    }
    else
    {
        g_WakeupSource = WAKEUP_SOURCE_NONE;
    }

    /* Re-enable peripherals */
    Power_EnablePeripherals();

    DEBUG_PRINT("Woke up from STOP mode (source: %d)\r\n", g_WakeupSource);

    return g_WakeupSource;
}

void Power_EnterSleepMode(void)
{
    /* Enter SLEEP mode (CPU stopped, peripherals running) */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void Power_ExitLowPowerMode(void)
{
    /* Restore system clock and peripherals */
    Power_SystemClockConfig_MSI();
    Power_EnablePeripherals();
}

void Power_DisablePeripherals(void)
{
    /* Disable UART to save power (except if AT command might come) */
    // Note: Keep UART enabled if you want to accept AT commands during sleep

    /* Disable GPIO clocks (except essential ones) */
    // Keep RTC clock enabled
}

void Power_EnablePeripherals(void)
{
    /* Re-enable UART */
    // UART will be re-initialized by board layer if needed

    /* Re-enable GPIO clocks */
}

void Power_RadioSleep(void)
{
    /* Put SX1276 in sleep mode */
    Radio.Sleep();

    /* Optionally disable TCXO to save more power */
    // GpioWrite(&TcxoPower, 0);
}

void Power_RadioWakeup(void)
{
    /* Wake up SX1276 from sleep */
    Radio.Standby();

    /* Re-enable TCXO if it was disabled */
    // GpioWrite(&TcxoPower, 1);
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

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY))
    {
        /* LSE is running, in STOP mode: estimate ~10µA */
        return 10;
    }

    /* In RUN mode: estimate ~1mA */
    return 1000;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ========================================================================== */

static void Power_ConfigureRTCWakeup(uint32_t wakeupTimeMs)
{
    /* Disable wake-up timer */
    HAL_RTCEx_DeactivateWakeUpTimer(&RtcHandle);

    /* Calculate wake-up counter value */
    /* RTC clock is LSE (32768 Hz), prescaler divides by 16 -> 2048 Hz */
    /* Counter = (wakeupTimeMs * 2048) / 1000 */
    uint32_t counter = (wakeupTimeMs * 2048) / 1000;

    /* Set wake-up timer */
    HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, counter, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

static void Power_SystemClockConfig_MSI(void)
{
    /* After wake-up from STOP, system clock is MSI (2.097 MHz) */
    /* Optionally switch to HSI (16 MHz) for better performance */

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    /* Enable HSI oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Select HSI as system clock source */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

    /* Update SystemCoreClock variable */
    SystemCoreClockUpdate();
}

/* ============================================================================
 * IRQ HANDLERS
 * ========================================================================== */

/*!
 * \brief RTC wake-up timer IRQ handler
 */
void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&RtcHandle);
}

/*!
 * \brief RTC wake-up timer callback
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* Wake-up event occurred */
    g_WakeupSource = WAKEUP_SOURCE_RTC;
}
