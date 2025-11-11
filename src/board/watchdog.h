/*!
 * \file      watchdog.h
 *
 * \brief     Independent Watchdog (IWDG) driver for STM32L072
 *
 * \details   Implements IWDG for system hang detection and automatic recovery.
 *            The IWDG uses the LSI clock (typically ~37 kHz) and continues
 *            running even in STOP mode, providing robust fault detection.
 *
 * \warning   Once enabled, the IWDG CANNOT be disabled by software!
 *            Only a system reset will stop it.
 */
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * CONFIGURATION
 * ========================================================================== */

/*!
 * \brief IWDG timeout in milliseconds
 *
 * \details With LSI @ 37 kHz, prescaler /256, reload 4095 (max):
 *          Timeout ≈ (4095 + 1) × 256 / 37000 ≈ 28.3 seconds
 *          Rounded to 32000ms for safety margin
 *
 * \note Matches OEM firmware configuration (32s timeout)
 *       This timeout must be longer than:
 *       - Maximum main loop iteration time
 *       - Maximum STOP mode sleep interval
 *       - LoRaWAN TX/RX cycle time (~15-20s worst case with retries)
 *
 * \warning OEM analysis confirmed 32s timeout is necessary to prevent
 *          spurious resets during extended TX operations
 */
#define WATCHDOG_TIMEOUT_MS         32000U

/*!
 * \brief IWDG refresh safety margin in milliseconds
 *
 * \details Refresh the watchdog at least this many milliseconds
 *          before the timeout expires to account for LSI variation.
 */
#define WATCHDOG_REFRESH_MARGIN_MS  2000U

/*!
 * \brief Maximum safe sleep time in STOP mode
 *
 * \details When entering STOP mode, if the requested sleep time exceeds
 *          this value, the system will wake up periodically to refresh
 *          the watchdog, then return to STOP mode.
 *
 *          With 32s timeout and 2s margin: Max STOP time = 30 seconds
 *          This allows for comfortable 60s TDC cycles with periodic refresh
 */
#define WATCHDOG_MAX_STOP_TIME_MS   (WATCHDOG_TIMEOUT_MS - WATCHDOG_REFRESH_MARGIN_MS)

/* ============================================================================
 * WATCHDOG RESET SOURCE
 * ========================================================================== */

/*!
 * \brief Watchdog reset source enumeration
 */
typedef enum
{
    WATCHDOG_RESET_NONE = 0,      /*!< No watchdog reset occurred */
    WATCHDOG_RESET_IWDG,          /*!< Reset caused by IWDG timeout */
    WATCHDOG_RESET_OTHER          /*!< Reset from other source */
} WatchdogResetSource_t;

/* ============================================================================
 * PUBLIC FUNCTIONS
 * ========================================================================== */

/*!
 * \brief Initialize and start the Independent Watchdog
 *
 * \details Configures the IWDG with:
 *          - LSI clock source (~37 kHz)
 *          - Prescaler: /128
 *          - Reload value: 3500
 *          - Timeout: ~12 seconds
 *
 * \warning Once this function is called, the watchdog CANNOT be stopped!
 *          It will continue running even in STOP mode and across resets
 *          (unless the option bytes are configured for software watchdog).
 *
 * \return true if initialization succeeded, false otherwise
 */
bool Watchdog_Init(void);

/*!
 * \brief Refresh the watchdog counter (kick the dog)
 *
 * \details Reloads the IWDG counter to prevent timeout reset.
 *          This function must be called at least once every
 *          WATCHDOG_TIMEOUT_MS milliseconds.
 *
 * \note This is a fast operation (~1-2 µs) and can be called frequently.
 *       It is safe to call this function before Watchdog_Init().
 */
void Watchdog_Refresh(void);

/*!
 * \brief Check if the last reset was caused by watchdog timeout
 *
 * \details Reads the RCC reset status register to determine if the
 *          Independent Watchdog triggered the last reset.
 *
 * \return Watchdog reset source
 *         - WATCHDOG_RESET_IWDG: Last reset was due to IWDG timeout
 *         - WATCHDOG_RESET_NONE: No watchdog reset
 *         - WATCHDOG_RESET_OTHER: Other reset source
 */
WatchdogResetSource_t Watchdog_GetResetSource(void);

/*!
 * \brief Clear the watchdog reset flags
 *
 * \details Clears the IWDG reset flag in RCC_CSR register.
 *          Should be called after reading the reset source to
 *          prepare for the next reset detection.
 */
void Watchdog_ClearResetFlags(void);

/*!
 * \brief Get the maximum safe STOP mode duration
 *
 * \details Returns the maximum time (in milliseconds) that the system
 *          can stay in STOP mode without needing to wake up and refresh
 *          the watchdog.
 *
 * \return Maximum safe STOP time in milliseconds
 */
static inline uint32_t Watchdog_GetMaxStopTime(void)
{
    return WATCHDOG_MAX_STOP_TIME_MS;
}

/*!
 * \brief Get the watchdog timeout value
 *
 * \details Returns the configured watchdog timeout in milliseconds.
 *
 * \return Watchdog timeout in milliseconds
 */
static inline uint32_t Watchdog_GetTimeout(void)
{
    return WATCHDOG_TIMEOUT_MS;
}

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_H__ */
