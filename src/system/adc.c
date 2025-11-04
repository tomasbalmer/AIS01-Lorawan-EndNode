/*!
 * \file      adc.c
 *
 * \brief     Generic ADC driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdbool.h>
#include <stddef.h>
#include "pinName-board.h"
#include "adc-board.h"

/*!
 * Flag to indicates if the ADC is initialized
 */
static bool AdcInitialized = false;

void AdcInit( Adc_t *obj, PinNames adcInput )
{
    if( ( obj == NULL ) || ( AdcInitialized == true ) )
    {
        return;
    }

    AdcMcuInit( obj, adcInput );
    AdcMcuConfig( );
    AdcInitialized = true;
}

void AdcDeInit( Adc_t *obj )
{
    if( ( obj == NULL ) || ( AdcInitialized == false ) )
    {
        return;
    }

    AdcMcuDeInit( obj );
    AdcInitialized = false;
}

uint16_t AdcReadChannel( Adc_t *obj, uint32_t channel )
{
    if( ( obj == NULL ) || ( AdcInitialized == false ) )
    {
        return 0;
    }

    return AdcMcuReadChannel( obj, channel );
}
