/*!
 * \file      pinName-board.h
 *
 * \brief     Pin name definitions for STM32L072CZ
 *
 * \details   This file defines all GPIO pin names for the STM32L072CZ MCU
 *            used in the Dragino AIS01-LB board.
 *
 * \remark    Pin naming convention:
 *            - Bits[7:4]: GPIO Port (0x0=A, 0x1=B, 0x2=C, 0x3=D, 0x4=E, 0x7=H)
 *            - Bits[3:0]: Pin number (0-15)
 */
#ifndef __PINNAME_BOARD_H__
#define __PINNAME_BOARD_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * STM32L072 Pin Names
 */
#define MCU_PIN_DEFS                                       \
    /* Port A */                                           \
    PA_0 = 0x00,                                           \
    PA_1 = 0x01,                                           \
    PA_2 = 0x02,                                           \
    PA_3 = 0x03,                                           \
    PA_4 = 0x04,                                           \
    PA_5 = 0x05,                                           \
    PA_6 = 0x06,                                           \
    PA_7 = 0x07,                                           \
    PA_8 = 0x08,                                           \
    PA_9 = 0x09,                                           \
    PA_10 = 0x0A,                                          \
    PA_11 = 0x0B,                                          \
    PA_12 = 0x0C,                                          \
    PA_13 = 0x0D,                                          \
    PA_14 = 0x0E,                                          \
    PA_15 = 0x0F, /* Port B */                             \
        PB_0 = 0x10,                                       \
    PB_1 = 0x11,                                           \
    PB_2 = 0x12,                                           \
    PB_3 = 0x13,                                           \
    PB_4 = 0x14,                                           \
    PB_5 = 0x15,                                           \
    PB_6 = 0x16,                                           \
    PB_7 = 0x17,                                           \
    PB_8 = 0x18,                                           \
    PB_9 = 0x19,                                           \
    PB_10 = 0x1A,                                          \
    PB_11 = 0x1B,                                          \
    PB_12 = 0x1C,                                          \
    PB_13 = 0x1D,                                          \
    PB_14 = 0x1E,                                          \
    PB_15 = 0x1F, /* Port C */                             \
        PC_0 = 0x20,                                       \
    PC_1 = 0x21,                                           \
    PC_2 = 0x22,                                           \
    PC_3 = 0x23,                                           \
    PC_4 = 0x24,                                           \
    PC_5 = 0x25,                                           \
    PC_6 = 0x26,                                           \
    PC_7 = 0x27,                                           \
    PC_8 = 0x28,                                           \
    PC_9 = 0x29,                                           \
    PC_10 = 0x2A,                                          \
    PC_11 = 0x2B,                                          \
    PC_12 = 0x2C,                                          \
    PC_13 = 0x2D,                                          \
    PC_14 = 0x2E,                                          \
    PC_15 = 0x2F, /* Port D (limited pins on STM32L072) */ \
        PD_0 = 0x30,                                       \
    PD_1 = 0x31,                                           \
    PD_2 = 0x32, /* Port E */                              \
        PE_0 = 0x40,                                       \
    PE_1 = 0x41,                                           \
    PE_2 = 0x42,                                           \
    PE_3 = 0x43,                                           \
    PE_4 = 0x44,                                           \
    PE_5 = 0x45,                                           \
    PE_6 = 0x46,                                           \
    PE_7 = 0x47,                                           \
    PE_8 = 0x48,                                           \
    PE_9 = 0x49,                                           \
    PE_10 = 0x4A,                                          \
    PE_11 = 0x4B,                                          \
    PE_12 = 0x4C,                                          \
    PE_13 = 0x4D,                                          \
    PE_14 = 0x4E,                                          \
    PE_15 = 0x4F, /* Port H (for external oscillator) */   \
        PH_0 = 0x70,                                       \
    PH_1 = 0x71

#define MCU_PINS MCU_PIN_DEFS

#ifdef __cplusplus
}
#endif

#endif /* __PINNAME_BOARD_H__ */
