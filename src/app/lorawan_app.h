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
        LORAWAN_APP_STATE_IDLE = 0,
        LORAWAN_APP_STATE_JOINING,
        LORAWAN_APP_STATE_JOINED,
        LORAWAN_APP_STATE_JOIN_FAILED,
        LORAWAN_APP_STATE_SENDING,
        LORAWAN_APP_STATE_SEND_SUCCESS,
        LORAWAN_APP_STATE_SEND_FAILED,
    } LoRaWANAppState_t;

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
     * \brief Sends a status uplink using the OEM formatter
     */
    bool LoRaWANApp_SendStatusUplink(void);

    /*!
     * \brief Sends a calibration uplink with raw calibration data
     */
    bool LoRaWANApp_SendCalibrationUplink(const uint8_t *calData, uint8_t calSize);

    /*!
     * \brief Sends a debug uplink frame
     */
    bool LoRaWANApp_SendDebugUplink(uint8_t fwMajor,
                                    uint8_t fwMinor,
                                    uint8_t fwPatch,
                                    uint8_t loraState);

    /*!
     * \brief Sends a sensor uplink using the OEM frame format
     */
    bool LoRaWANApp_SendSensorUplink(void);

    /*!
     * \brief Sends an OEM Sensor-Stats uplink frame
     */
    bool LoRaWANApp_SendSensorStatsUplink(void);

    /*!
     * \brief Sends an Expanded OEM Status uplink frame
     */
    bool LoRaWANApp_SendStatusExUplink(void);

    /*!
     * \brief Sends an OEM MAC-mirror uplink frame
     */
    bool LoRaWANApp_SendMacMirrorUplink(void);

    /*!
     * \brief Sends OEM Power Profile frame (0xF3)
     */
    bool LoRaWANApp_SendPowerProfileUplink(void);

    /*!
     * \brief Processes LoRaMAC events (must be called in main loop)
     */
    void LoRaWANApp_Process(void);

    /*!
     * \brief Gets current LoRaWAN status
     * \retval Current status
     */
    LoRaWANAppState_t LoRaWANApp_GetStatus(void);

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
