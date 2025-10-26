/*!
 * \file      delay-board.h
 *
 * \brief     Target board delay driver definitions
 */
#ifndef __DELAY_BOARD_H__
#define __DELAY_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*!
 * \brief Blocking delay in milliseconds
 *
 * \param [IN] ms Delay in milliseconds
 */
void DelayMsMcu(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_BOARD_H__ */
