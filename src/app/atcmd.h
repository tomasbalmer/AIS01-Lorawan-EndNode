/*!
 * \file      atcmd.h
 *
 * \brief     AT command parser and dispatcher
 *
 * \details   Implements AT command interface for LoRaWAN configuration
 *            and device control via UART.
 */
#ifndef __ATCMD_H__
#define __ATCMD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * AT COMMAND RESULT CODES
 * ========================================================================== */
typedef enum
{
    ATCMD_OK = 0,
    ATCMD_ERROR,
    ATCMD_INVALID_PARAM,
    ATCMD_NOT_FOUND,
} ATCmdResult_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ========================================================================== */

/*!
 * \brief Initializes AT command parser
 * \retval true if initialization successful
 */
bool ATCmd_Init(void);

/*!
 * \brief Processes received AT command string
 * \param [in] cmdLine Command line string
 * \retval Command execution result
 */
ATCmdResult_t ATCmd_Process(const char *cmdLine);

/*!
 * \brief Processes incoming UART data (character by character)
 * \param [in] rxChar Received character
 */
void ATCmd_ProcessChar(uint8_t rxChar);

/*!
 * \brief Sends AT command response
 * \param [in] response Response string
 */
void ATCmd_SendResponse(const char *response);

/*!
 * \brief Sends formatted AT command response
 * \param [in] format Printf-style format string
 * \param [in] ... Variable arguments
 */
void ATCmd_SendFormattedResponse(const char *format, ...);

/*!
 * \brief Updates last received RSSI value
 * \param [in] rssi RSSI value in dBm
 */
void ATCmd_UpdateRSSI(int16_t rssi);

/*!
 * \brief Updates last received SNR value
 * \param [in] snr SNR value in dB
 */
void ATCmd_UpdateSNR(int8_t snr);

/*!
 * \brief Updates pending downlink status
 * \param [in] pending Number of pending downlinks
 */
void ATCmd_UpdatePendingDownlink(uint8_t pending);

/*!
 * \brief Updates confirmed message status
 * \param [in] status 0=pending, 1=success, 2=failed
 */
void ATCmd_UpdateConfirmedStatus(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* __ATCMD_H__ */
