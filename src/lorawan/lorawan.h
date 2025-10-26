#ifndef LORAWAN_H
#define LORAWAN_H

#include <stdint.h>
#include <stdbool.h>
#include "lorawan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void (*OnJoinSuccess)(uint32_t devAddr);
    void (*OnJoinFailure)(void);
    void (*OnTxComplete)(LoRaWANStatus_t status);
    void (*OnRxData)(const uint8_t *buffer, uint8_t size, uint8_t port, int16_t rssi, int8_t snr);
} LoRaWANCallbacks_t;

typedef struct
{
    LoRaWANSession_t *Session;
    LoRaWANSettings_t Settings;
    LoRaWANCallbacks_t Callbacks;
    uint8_t *RadioBuffer;
    uint16_t RadioBufferSize;
} LoRaWANContext_t;

LoRaWANStatus_t LoRaWAN_Init(LoRaWANContext_t *ctx);
LoRaWANStatus_t LoRaWAN_RequestJoin(LoRaWANContext_t *ctx);
LoRaWANStatus_t LoRaWAN_Send(LoRaWANContext_t *ctx, const uint8_t *buffer, uint8_t size, uint8_t port, LoRaWANMsgType_t msgType);
void LoRaWAN_Process(LoRaWANContext_t *ctx);
void LoRaWAN_RunRxWindow(LoRaWANContext_t *ctx, uint8_t window); /* window: 1 or 2 */
void LoRaWAN_HandleRadioEvent(LoRaWANContext_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_H */
