#ifndef LORAWAN_CRYPTO_H
#define LORAWAN_CRYPTO_H

#include <stdint.h>
#include <stdbool.h>

bool LoRaWAN_Crypto_ComputeJoinMic(const uint8_t *appKey, const uint8_t *joinRequest, uint8_t length, uint32_t *mic);
bool LoRaWAN_Crypto_ComputeJoinKeys(const uint8_t *appKey, const uint8_t *appNonce, const uint8_t *netId, const uint8_t *devNonce, uint8_t *nwkSKey, uint8_t *appSKey);
bool LoRaWAN_Crypto_ComputeMic(const uint8_t *nwkSKey, const uint8_t *payload, uint8_t length, uint32_t devAddr, uint32_t fCnt, uint8_t direction, uint32_t *mic);
bool LoRaWAN_Crypto_EncryptPayload(const uint8_t *key, const uint8_t *input, uint8_t length, uint32_t devAddr, uint32_t fCnt, uint8_t direction, uint8_t *output);

#endif /* LORAWAN_CRYPTO_H */
