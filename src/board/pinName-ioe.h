/*!
 * \file      pinName-ioe.h
 *
 * \brief     Pin name definitions for IO Expander
 *
 * \details   This file defines GPIO pin names for IO expander chips.
 *            The Dragino AIS01-LB does not use an IO expander, so this
 *            file provides stub definitions for compatibility with the
 *            LoRaMAC-node stack.
 *
 * \remark    IO expander pins start at 0x60 to avoid conflicts with MCU pins
 */
#ifndef __PINNAME_IOE_H__
#define __PINNAME_IOE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * IO Expander Pin Names (stub definitions)
 */
#define IOE_PIN_DEFS                              \
    /* IO Expander pins (not used in AIS01-LB) */ \
    IOE_0 = 0x60,                                 \
    IOE_1 = 0x61,                                 \
    IOE_2 = 0x62,                                 \
    IOE_3 = 0x63,                                 \
    IOE_4 = 0x64,                                 \
    IOE_5 = 0x65,                                 \
    IOE_6 = 0x66,                                 \
    IOE_7 = 0x67,                                 \
    IOE_8 = 0x68,                                 \
    IOE_9 = 0x69,                                 \
    IOE_10 = 0x6A,                                \
    IOE_11 = 0x6B,                                \
    IOE_12 = 0x6C,                                \
    IOE_13 = 0x6D,                                \
    IOE_14 = 0x6E,                                \
    IOE_15 = 0x6F

#define IOE_PINS IOE_PIN_DEFS

#ifdef __cplusplus
}
#endif

#endif
