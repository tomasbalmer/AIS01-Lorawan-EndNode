/*!
 * \file      gps-board.h
 *
 * \brief     Target board GPS driver definitions
 *
 * \remark    AIS01-LB does not have GPS hardware - this is a stub
 */
#ifndef __GPS_BOARD_H__
#define __GPS_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "pinName-board.h"
#include "eeprom-board.h"

/*!
 * \brief Initializes the GPS
 */
void GpsMcuInit( void );

/*!
 * \brief DeInitializes the GPS
 */
void GpsMcuDeInit( void );

/*!
 * \brief Starts the GPS
 */
void GpsMcuStart( void );

/*!
 * \brief Stops the GPS
 */
void GpsMcuStop( void );

/*!
 * \brief Updates the GPS process
 */
void GpsMcuProcess( void );

/*!
 * \brief IRQ handler for GPS UART
 */
void GpsMcuIrqHandler( void );

#ifdef __cplusplus
}
#endif

#endif /* __GPS_BOARD_H__ */
