/*!
 * \file      lpm-board.h
 *
 * \brief     Low power mode (LPM) control interface for the AIS01-LB board.
 */
#ifndef __LPM_BOARD_H__
#define __LPM_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*!\brief Identifiers assigned to subsystems that can veto low power modes */
typedef enum
{
    LPM_APPLI_ID = ( 1U << 0 ),
    LPM_RTC_ID   = ( 1U << 1 ),
    LPM_RADIO_ID = ( 1U << 2 ),
} LpmId_t;

/*!\brief Enable/disable flag for low power modes */
typedef enum
{
    LPM_DISABLE = 0,
    LPM_ENABLE  = 1,
} LpmSetMode_t;

/*!\brief Supported low power operating modes */
typedef enum
{
    LPM_SLEEP_MODE = 0,
    LPM_STOP_MODE,
    LPM_OFF_MODE,
} LpmGetMode_t;

void LpmSetOffMode( LpmId_t id, LpmSetMode_t mode );
void LpmSetStopMode( LpmId_t id, LpmSetMode_t mode );
void LpmEnterLowPower( void );
LpmGetMode_t LpmGetMode( void );

void LpmEnterSleepMode( void );
void LpmExitSleepMode( void );
void LpmEnterStopMode( void );
void LpmExitStopMode( void );
void LpmEnterOffMode( void );
void LpmExitOffMode( void );

#ifdef __cplusplus
}
#endif

#endif // __LPM_BOARD_H__
