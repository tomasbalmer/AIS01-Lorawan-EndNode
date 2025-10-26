/*!
 * \file      utilities.h
 *
 * \brief     Common utility macros and helper functions used across the stack.
 */
#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*!\brief Macro used to avoid compiler warnings about unused variables */
#ifndef UNUSED
#define UNUSED(x)                       (void)(x)
#endif

#ifndef MIN
#define MIN(a, b)                       (( (a) < (b) ) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)                       (( (a) > (b) ) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)                          (((a) < 0) ? -(a) : (a))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)                   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef POW2
#define POW2(n)                         (1UL << (n))
#endif

/*!
 * \brief Critical section management helpers
 */
void BoardCriticalSectionBegin( uint32_t *mask );
void BoardCriticalSectionEnd( uint32_t *mask );

#define CRITICAL_SECTION_BEGIN( )           \
    uint32_t primask;                       \
    BoardCriticalSectionBegin( &primask );

#define CRITICAL_SECTION_END( )             \
    BoardCriticalSectionEnd( &primask )

/*!
 * \brief Memory helper functions
 */
void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size );
void memset1( uint8_t *dst, uint8_t value, uint16_t size );

/*!
 * \brief Converts a nibble to the corresponding ASCII hex character.
 *
 * \param [IN] a  Nibble to be converted (0-15).
 * \retval Hex character ("0"-"9", "A"-"F") or '?' if out of range.
 */
int8_t Nibble2HexChar( uint8_t a );

#ifdef __cplusplus
}
#endif

#endif // __UTILITIES_H__
