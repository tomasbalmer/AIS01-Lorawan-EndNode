/*!
 * \file      i2c-board.h
 *
 * \brief     Target board I2C driver definitions
 */
#ifndef __I2C_BOARD_H__
#define __I2C_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "pinName-board.h"
#include "eeprom-board.h"
#include "i2c.h"

#define MODE_I2C                0
#define I2C_DUTY_CYCLE_2        0
#define I2C_ACK_ADD_7_BIT       0

void I2cMcuInit( I2c_t *obj, I2cId_t i2cId, PinNames scl, PinNames sda );
void I2cMcuDeInit( I2c_t *obj );
void I2cMcuFormat( I2c_t *obj, I2cMode mode, I2cDutyCycle dutyCycle, bool I2cAckEnable, I2cAckAddrMode AckAddrMode, uint32_t I2cFrequency );
void I2cMcuResetBus( I2c_t *obj );
LmnStatus_t I2cMcuWriteBuffer( I2c_t *obj, uint8_t deviceAddr, uint8_t *buffer, uint16_t size );
LmnStatus_t I2cMcuReadBuffer( I2c_t *obj, uint8_t deviceAddr, uint8_t *buffer, uint16_t size );
LmnStatus_t I2cMcuWriteMemBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size );
LmnStatus_t I2cMcuReadMemBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size );

#ifdef __cplusplus
}
#endif

#endif /* __I2C_BOARD_H__ */
