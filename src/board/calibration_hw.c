/*!
 * \file      calibration_hw.c
 *
 * \brief     Hardware glue for calibration apply engine
 *
 * \details   Implements the low-level hooks that the calibration module uses
 *            to synchronise with the hardware configuration registers. These
 *            addresses and semantics are derived from the stock firmware
 *            reverse-engineering notes.
 */
#include "calibration.h"
#include "stm32l0xx.h"

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) do { } while (0)
#endif

#define CALIB_READY_REG          (*((volatile uint32_t *)0x4001D000u))
#define CALIB_READY_MASK         (0x0Fu)

#define CALIB_ENABLE_REG         (*((volatile uint32_t *)0x40000024u))
#define CALIB_ENABLE_BIT         (1u << 5)

#define CALIB_HW_TIMEOUT         (100000u)

bool Calibration_HwWaitReady(void)
{
    for (uint32_t i = 0; i < CALIB_HW_TIMEOUT; ++i)
    {
        if ((CALIB_READY_REG & CALIB_READY_MASK) == CALIB_READY_MASK)
        {
            return true;
        }
    }

    DEBUG_PRINT("Calibration: hardware ready timeout\r\n");
    return false;
}

void Calibration_HwSetEnable(bool enable)
{
    uint32_t reg = CALIB_ENABLE_REG;

    if (enable)
    {
        reg |= CALIB_ENABLE_BIT;
    }
    else
    {
        reg &= ~CALIB_ENABLE_BIT;
    }

    CALIB_ENABLE_REG = reg;
}
