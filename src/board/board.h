/*!
 * \file      board.h
 *
 * \brief     Target board general functions definitions
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*!
 * Board power sources
 */
typedef enum
{
    USB_POWER = 0,
    BATTERY_POWER
} BoardPowerSource_t;

/*!
 * Critical section management
 */
void BoardCriticalSectionBegin( uint32_t *mask );
void BoardCriticalSectionEnd( uint32_t *mask );

/*!
 * Board initialisation and power control
 */
void BoardInitMcu( void );
void BoardInitPeriph( void );
void BoardDeInitMcu( void );
void BoardResetMcu( void );
void BoardLowPowerHandler( void );

/*!
 * Miscellaneous helpers
 */
uint32_t BoardGetRandomSeed( void );
void BoardGetUniqueId( uint8_t *id );
uint16_t BoardBatteryMeasureVoltage( void );
uint32_t BoardGetBatteryVoltage( void );
uint8_t BoardGetBatteryLevel( void );
uint8_t GetBoardPowerSource( void );

#ifdef __cplusplus
}
#endif

#endif // __BOARD_H__
