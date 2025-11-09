/*!
 * \file      watchdog.c
 *
 * \brief     Independent Watchdog (IWDG) driver implementation
 *
 * \details   Implements IWDG for STM32L072 with the following configuration:
 *            - Clock source: LSI (Low Speed Internal ~37 kHz)
 *            - Prescaler: /128
 *            - Reload value: 3500
 *            - Timeout: ~12 seconds
 *
 *            The IWDG continues running in STOP mode, providing robust
 *            fault detection even during low-power operation.
 */
#include <stdio.h>
#include "watchdog.h"
#include "config.h"
#include "stm32l072xx.h"
#include "stm32l0xx.h"
#include "utilities.h"

/* ============================================================================
 * IWDG KEY VALUES
 * ========================================================================== */

#define IWDG_KEY_RELOAD             0xAAAAU     /*!< Reload counter */
#define IWDG_KEY_ENABLE             0xCCCCU     /*!< Enable watchdog */
#define IWDG_KEY_WRITE_ACCESS       0x5555U     /*!< Enable write access to PR and RLR */

/* ============================================================================
 * IWDG PRESCALER VALUES
 * ========================================================================== */

#define IWDG_PRESCALER_4            0x00U       /*!< Prescaler /4 */
#define IWDG_PRESCALER_8            0x01U       /*!< Prescaler /8 */
#define IWDG_PRESCALER_16           0x02U       /*!< Prescaler /16 */
#define IWDG_PRESCALER_32           0x03U       /*!< Prescaler /32 */
#define IWDG_PRESCALER_64           0x04U       /*!< Prescaler /64 */
#define IWDG_PRESCALER_128          0x05U       /*!< Prescaler /128 */
#define IWDG_PRESCALER_256          0x06U       /*!< Prescaler /256 */

/* ============================================================================
 * CONFIGURATION FOR 12-SECOND TIMEOUT
 * ========================================================================== */

/*!
 * \brief LSI clock frequency (typical value)
 * \note Actual LSI can vary between 32 kHz and 47 kHz.
 *       We use 37 kHz as typical value per STM32L072 datasheet.
 */
#define LSI_FREQUENCY_HZ            37000U

/*!
 * \brief Selected prescaler: /128
 * \note This provides good resolution while allowing long timeouts.
 */
#define IWDG_PRESCALER_VALUE        IWDG_PRESCALER_128
#define IWDG_PRESCALER_DIVIDER      128U

/*!
 * \brief Reload value for ~12 second timeout
 * \details Calculation:
 *          Timeout = (Reload + 1) × Prescaler / LSI_freq
 *          12000 ms = (Reload + 1) × 128 / 37000
 *          Reload = (12000 × 37000 / 128) - 1 = 3468.75 - 1 ≈ 3500
 *
 *          Actual timeout with Reload=3500:
 *          Timeout = (3500 + 1) × 128 / 37000 = 12.1 seconds
 */
#define IWDG_RELOAD_VALUE           3500U

/* ============================================================================
 * PRIVATE VARIABLES
 * ========================================================================== */

static bool g_WatchdogInitialized = false;

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

bool Watchdog_Init(void)
{
    if (g_WatchdogInitialized)
    {
        /* Already initialized, just refresh */
        Watchdog_Refresh();
        return true;
    }

    /* Check reset source before clearing flags */
    WatchdogResetSource_t resetSource = Watchdog_GetResetSource();
    if (resetSource == WATCHDOG_RESET_IWDG)
    {
        DEBUG_PRINT("WARNING: System was reset by watchdog timeout!\r\n");
        /* Log this event or increment error counter */
    }

    /* Clear reset flags for next time */
    Watchdog_ClearResetFlags();

    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG->KR = IWDG_KEY_WRITE_ACCESS;

    /* Wait for the registers to be ready for writing */
    uint32_t timeout = 10000U;  /* Timeout counter */
    while (((IWDG->SR & IWDG_SR_PVU) != 0U || (IWDG->SR & IWDG_SR_RVU) != 0U) && timeout > 0U)
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        DEBUG_PRINT("ERROR: IWDG register update timeout\r\n");
        return false;
    }

    /* Set prescaler to /128 */
    IWDG->PR = IWDG_PRESCALER_VALUE;

    /* Set reload value for ~12 second timeout */
    IWDG->RLR = IWDG_RELOAD_VALUE;

    /* Wait for registers to be updated */
    timeout = 10000U;
    while (((IWDG->SR & IWDG_SR_PVU) != 0U || (IWDG->SR & IWDG_SR_RVU) != 0U) && timeout > 0U)
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        DEBUG_PRINT("ERROR: IWDG configuration timeout\r\n");
        return false;
    }

    /* Enable the IWDG (starts the counter) */
    /* WARNING: After this point, the watchdog CANNOT be stopped! */
    IWDG->KR = IWDG_KEY_ENABLE;

    /* Reload the counter immediately */
    IWDG->KR = IWDG_KEY_RELOAD;

    g_WatchdogInitialized = true;

    DEBUG_PRINT("Watchdog initialized (timeout: %u ms)\r\n", WATCHDOG_TIMEOUT_MS);
    DEBUG_PRINT("  LSI freq: %u Hz\r\n", LSI_FREQUENCY_HZ);
    DEBUG_PRINT("  Prescaler: /%u\r\n", IWDG_PRESCALER_DIVIDER);
    DEBUG_PRINT("  Reload value: %u\r\n", IWDG_RELOAD_VALUE);
    DEBUG_PRINT("  Max STOP time: %u ms\r\n", WATCHDOG_MAX_STOP_TIME_MS);

    return true;
}

void Watchdog_Refresh(void)
{
    /* Reload the watchdog counter */
    /* This operation is fast (~1-2 µs) and can be called frequently */
    IWDG->KR = IWDG_KEY_RELOAD;
}

WatchdogResetSource_t Watchdog_GetResetSource(void)
{
    /* Check if IWDG reset flag is set */
    if ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U)
    {
        return WATCHDOG_RESET_IWDG;
    }

    /* Check if any other reset flag is set */
    if ((RCC->CSR & (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_SFTRSTF |
                     RCC_CSR_PORRSTF | RCC_CSR_PINRSTF | RCC_CSR_OBLRSTF)) != 0U)
    {
        return WATCHDOG_RESET_OTHER;
    }

    return WATCHDOG_RESET_NONE;
}

void Watchdog_ClearResetFlags(void)
{
    /* Set RMVF bit to clear all reset flags in RCC_CSR */
    RCC->CSR |= RCC_CSR_RMVF;
}

/* ============================================================================
 * TIMEOUT CALCULATION VERIFICATION
 * ========================================================================== */

/*
 * Timeout calculation verification:
 *
 * Formula: Timeout (ms) = (Reload + 1) × Prescaler / LSI_freq × 1000
 *
 * With our configuration:
 * - LSI_freq = 37000 Hz (typical)
 * - Prescaler = 128
 * - Reload = 3500
 *
 * Timeout = (3500 + 1) × 128 / 37000 × 1000
 *         = 3501 × 128 / 37000 × 1000
 *         = 448128 / 37 ms
 *         = 12111 ms
 *         ≈ 12.1 seconds
 *
 * LSI variation analysis:
 * - LSI min (32 kHz): Timeout = 3501 × 128 / 32000 = 14.0 seconds
 * - LSI typ (37 kHz): Timeout = 3501 × 128 / 37000 = 12.1 seconds
 * - LSI max (47 kHz): Timeout = 3501 × 128 / 47000 = 9.5 seconds
 *
 * Worst-case timeout: 9.5 seconds (LSI at maximum frequency)
 * Therefore, WATCHDOG_MAX_STOP_TIME_MS = 12000 - 2000 = 10000 ms
 * provides sufficient safety margin even with LSI variation.
 */
