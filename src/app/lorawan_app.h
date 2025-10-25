/*!
 * \file      lorawan_app.h
 *
 * \brief     LoRaWAN application layer
 *
 * \details   Handles LoRaMAC initialization, OTAA join, uplinks, downlinks,
 *            and callbacks for AU915 region.
 */
#ifndef __LORAWAN_APP_H__
#define __LORAWAN_APP_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * LORAWAN APPLICATION STATUS
 * ========================================================================== */
typedef enum
{
    LORAWAN_STATUS_IDLE = 0,
    LORAWAN_STATUS_JOINING,
    LORAWAN_STATUS_JOINED,
    LORAWAN_STATUS_JOIN_FAILED,
    LORAWAN_STATUS_SENDING,
    LORAWAN_STATUS_SEND_SUCCESS,
    LORAWAN_STATUS_SEND_FAILED,
} LoRaWANStatus_t;

/* ============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ========================================================================== */

/*!
 * \brief Initializes LoRaWAN stack and AU915 region
 * \retval true if initialization successful
 */
bool LoRaWANApp_Init(void);

/*!
 * \brief Initiates OTAA join procedure
 * \retval true if join request sent successfully
 */
bool LoRaWANApp_Join(void);

/*!
 * \brief Sends uplink message
 * \param [in] buffer Data buffer to send
 * \param [in] size Size of data
 * \param [in] port Application port
 * \param [in] confirmed true for confirmed, false for unconfirmed
 * \retval true if send request successful
 */
bool LoRaWANApp_SendUplink(uint8_t *buffer, uint8_t size, uint8_t port, bool confirmed);

/*!
 * \brief Processes LoRaMAC events (must be called in main loop)
 */
void LoRaWANApp_Process(void);

/*!
 * \brief Gets current LoRaWAN status
 * \retval Current status
 */
LoRaWANStatus_t LoRaWANApp_GetStatus(void);

/*!
 * \brief Checks if device is joined to network
 * \retval true if joined
 */
bool LoRaWANApp_IsJoined(void);

/*!
 * \brief Gets DevAddr (only valid after join)
 * \retval DevAddr
 */
uint32_t LoRaWANApp_GetDevAddr(void);

#ifdef __cplusplus
}
#endif

#endif /* __LORAWAN_APP_H__ */
