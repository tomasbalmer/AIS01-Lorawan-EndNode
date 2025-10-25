/*!
 * \file      power.h
 *
 * \brief     Power management module
 *
 * \details   Handles ultra-low power modes (STOP mode), RTC wake-up,
 *            clock gating, and radio shutdown to achieve <20µA in sleep.
 */
#ifndef __POWER_H__
#define __POWER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * POWER MODE DEFINITIONS
 * ========================================================================== */
typedef enum
{
    POWER_MODE_RUN = 0,               /* Normal run mode */
    POWER_MODE_SLEEP,                 /* Sleep mode (CPU stopped, peripherals running) */
    POWER_MODE_STOP,                  /* STOP mode (ultra-low power, RTC running) */
} PowerMode_t;

/* ============================================================================
 * WAKE-UP SOURCE DEFINITIONS
 * ========================================================================== */
typedef enum
{
    WAKEUP_SOURCE_NONE = 0,
    WAKEUP_SOURCE_RTC,                /* RTC wake-up timer */
    WAKEUP_SOURCE_UART,               /* UART activity */
    WAKEUP_SOURCE_RADIO,              /* Radio interrupt */
    WAKEUP_SOURCE_BUTTON,             /* Button press */
} WakeupSource_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ========================================================================== */

/*!
 * \brief Initializes power management module
 * \retval true if initialization successful
 */
bool Power_Init(void);

/*!
 * \brief Enters STOP mode with RTC wake-up
 * \param [in] wakeupTimeMs Wake-up time in milliseconds
 * \retval Wake-up source
 */
WakeupSource_t Power_EnterStopMode(uint32_t wakeupTimeMs);

/*!
 * \brief Enters SLEEP mode
 */
void Power_EnterSleepMode(void);

/*!
 * \brief Returns from low power mode to RUN mode
 */
void Power_ExitLowPowerMode(void);

/*!
 * \brief Disables all non-essential peripherals to save power
 */
void Power_DisablePeripherals(void);

/*!
 * \brief Re-enables peripherals after wake-up
 */
void Power_EnablePeripherals(void);

/*!
 * \brief Puts SX1276 radio in sleep mode
 */
void Power_RadioSleep(void);

/*!
 * \brief Wakes up SX1276 radio from sleep mode
 */
void Power_RadioWakeup(void);

/*!
 * \brief Gets the last wake-up source
 * \retval Last wake-up source
 */
WakeupSource_t Power_GetWakeupSource(void);

/*!
 * \brief Measures current consumption (for debugging)
 * \retval Estimated current in µA
 */
uint32_t Power_MeasureCurrentConsumption(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_H__ */
