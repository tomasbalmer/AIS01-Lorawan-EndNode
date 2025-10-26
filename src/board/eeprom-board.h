/*!
 * \file      eeprom-board.h
 *
 * \brief     EEPROM emulation in Flash memory for STM32L072CZ
 *
 * \details   Provides low-level read/write functions for internal Flash
 *            used as non-volatile EEPROM storage.
 */
#ifndef __EEPROM_BOARD_H__
#define __EEPROM_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*!
 * \brief LMN Status enumeration for return values
 */
typedef enum
{
    LMN_STATUS_OK = 0,
    LMN_STATUS_ERROR
} LmnStatus_t;

/*!
 * \brief Writes data to EEPROM (Flash emulation)
 *
 * \param [in] addr    EEPROM address offset
 * \param [in] buffer  Pointer to data buffer to write
 * \param [in] size    Number of bytes to write
 *
 * \retval LMN_STATUS_OK      Write successful
 * \retval LMN_STATUS_ERROR   Write failed
 */
LmnStatus_t EepromMcuWriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t size);

/*!
 * \brief Reads data from EEPROM (Flash emulation)
 *
 * \param [in]  addr    EEPROM address offset
 * \param [out] buffer  Pointer to buffer to store read data
 * \param [in]  size    Number of bytes to read
 *
 * \retval LMN_STATUS_OK      Read successful
 * \retval LMN_STATUS_ERROR   Read failed
 */
LmnStatus_t EepromMcuReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t size);

/*!
 * \brief Sets the EEPROM device address (placeholder)
 *
 * \param [in] addr Device address (currently unused)
 */
void EepromMcuSetDeviceAddr(uint8_t addr);

/*!
 * \brief Gets the EEPROM device address
 *
 * \retval Device address (currently returns 0)
 */
LmnStatus_t EepromMcuGetDeviceAddr(void);

#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_BOARD_H__ */
