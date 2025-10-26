/*!
 * \file      utilities.c
 *
 * \brief     Common utility helper implementations used across the stack.
 */
#include "utilities.h"

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    if( ( dst == NULL ) || ( src == NULL ) || ( size == 0 ) )
    {
        return;
    }

    while( size-- )
    {
        *dst++ = *src++;
    }
}

void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    if( ( dst == NULL ) || ( size == 0 ) )
    {
        return;
    }

    while( size-- )
    {
        *dst++ = value;
    }
}

int8_t Nibble2HexChar( uint8_t a )
{
    if( a < 10U )
    {
        return ( int8_t )( '0' + a );
    }
    if( a < 16U )
    {
        return ( int8_t )( 'A' + ( a - 10U ) );
    }
    return '?';
}
