/*!
 * \file      sx1276-board.h
 *
 * \brief     Board specific SX1276 driver helpers.
 */
#ifndef __SX1276_BOARD_H__
#define __SX1276_BOARD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "radio.h"
#include "gpio.h"
#include "sx1276/sx1276.h"

    /*!\brief External radio driver instance */
    extern const struct Radio_s Radio;

    void SX1276IoInit(void);
    void SX1276IoIrqInit(DioIrqHandler **irqHandlers);
    void SX1276IoDeInit(void);
    void SX1276IoDbgInit(void);
    void SX1276IoTcxoInit(void);
    void SX1276SetBoardTcxo(uint8_t state);
    uint32_t SX1276GetBoardTcxoWakeupTime(void);
    void SX1276Reset(void);
    void SX1276SetRfTxPower(int8_t power);
    void SX1276SetAntSwLowPower(bool status);
    void SX1276AntSwInit(void);
    void SX1276AntSwDeInit(void);
    void SX1276SetAntSw(uint8_t opMode);
    void SX1276DbgPinTxWrite(uint8_t state);
    void SX1276DbgPinRxWrite(uint8_t state);

    /*!
     * Radio registers initialization structure
     */
    typedef struct
    {
        uint8_t Modem;
        uint8_t Addr;
        uint8_t Value;
    } RadioRegisters_t;

/*!
 * Radio registers initialization values
 * This is a minimal set - registers will be configured as needed during operation
 */
#define RADIO_INIT_REGISTERS_VALUE \
    {                              \
    }

#ifdef __cplusplus
}
#endif

#endif // __SX1276_BOARD_H__
