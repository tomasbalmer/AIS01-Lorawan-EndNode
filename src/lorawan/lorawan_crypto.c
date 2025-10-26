#include <string.h>
#include "lorawan_crypto.h"
#include "lorawan_types.h"
#include "aes.h"
#include "cmac.h"

#define LORAWAN_BLOCK_SIZE 16

static void PrepareB0(uint8_t *b0, uint8_t direction, uint32_t devAddr, uint32_t fCnt, uint8_t len)
{
    memset(b0, 0, LORAWAN_BLOCK_SIZE);
    b0[0] = 0x49;
    b0[5] = direction;
    b0[6] = (uint8_t)(devAddr & 0xFF);
    b0[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    b0[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    b0[9] = (uint8_t)((devAddr >> 24) & 0xFF);
    b0[10] = (uint8_t)(fCnt & 0xFF);
    b0[11] = (uint8_t)((fCnt >> 8) & 0xFF);
    b0[12] = (uint8_t)((fCnt >> 16) & 0xFF);
    b0[13] = (uint8_t)((fCnt >> 24) & 0xFF);
    b0[15] = len;
}

static void PrepareAi(uint8_t *a, uint8_t direction, uint32_t devAddr, uint32_t fCnt, uint16_t counter)
{
    memset(a, 0, LORAWAN_BLOCK_SIZE);
    a[0] = 0x01;
    a[5] = direction;
    a[6] = (uint8_t)(devAddr & 0xFF);
    a[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    a[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    a[9] = (uint8_t)((devAddr >> 24) & 0xFF);
    a[10] = (uint8_t)(fCnt & 0xFF);
    a[11] = (uint8_t)((fCnt >> 8) & 0xFF);
    a[12] = (uint8_t)((fCnt >> 16) & 0xFF);
    a[13] = (uint8_t)((fCnt >> 24) & 0xFF);
    a[15] = (uint8_t)(counter & 0xFF);
}

bool LoRaWAN_Crypto_ComputeJoinMic(const uint8_t *appKey, const uint8_t *joinRequest,
                                   uint8_t length, uint32_t *mic)
{
    if (appKey == NULL || joinRequest == NULL || mic == NULL || length == 0)
    {
        return false;
    }

    AES_CMAC_CTX ctx;
    uint8_t digest[AES_CMAC_DIGEST_LENGTH];

    AES_CMAC_Init(&ctx);
    AES_CMAC_SetKey(&ctx, appKey);
    AES_CMAC_Update(&ctx, joinRequest, length);
    AES_CMAC_Final(digest, &ctx);

    *mic = (uint32_t)digest[0] | ((uint32_t)digest[1] << 8) |
           ((uint32_t)digest[2] << 16) | ((uint32_t)digest[3] << 24);
    return true;
}

bool LoRaWAN_Crypto_ComputeJoinKeys(const uint8_t *appKey, const uint8_t *appNonce,
                                    const uint8_t *netId, const uint8_t *devNonce,
                                    uint8_t *nwkSKey, uint8_t *appSKey)
{
    if (appKey == NULL || appNonce == NULL || netId == NULL || devNonce == NULL ||
        nwkSKey == NULL || appSKey == NULL)
    {
        return false;
    }

    uint8_t buffer[16];
    aes_context ctx;

    if (aes_set_key(appKey, 16, &ctx) != 0)
    {
        return false;
    }

    buffer[0] = 0x01;
    memcpy(&buffer[1], appNonce, 3);
    memcpy(&buffer[4], netId, 3);
    memcpy(&buffer[7], devNonce, 2);
    memset(&buffer[9], 0, 7);

    if (aes_encrypt(buffer, nwkSKey, &ctx) != 0)
    {
        return false;
    }

    buffer[0] = 0x02;
    if (aes_encrypt(buffer, appSKey, &ctx) != 0)
    {
        return false;
    }

    return true;
}

bool LoRaWAN_Crypto_ComputeMic(const uint8_t *nwkSKey, const uint8_t *payload,
                               uint8_t length, uint32_t devAddr, uint32_t fCnt,
                               uint8_t direction, uint32_t *mic)
{
    if (nwkSKey == NULL || payload == NULL || mic == NULL)
    {
        return false;
    }

    uint8_t b0[LORAWAN_BLOCK_SIZE];
    uint8_t digest[AES_CMAC_DIGEST_LENGTH];
    AES_CMAC_CTX ctx;

    PrepareB0(b0, direction, devAddr, fCnt, length);

    AES_CMAC_Init(&ctx);
    AES_CMAC_SetKey(&ctx, nwkSKey);
    AES_CMAC_Update(&ctx, b0, LORAWAN_BLOCK_SIZE);
    AES_CMAC_Update(&ctx, payload, length);
    AES_CMAC_Final(digest, &ctx);

    *mic = (uint32_t)digest[0] | ((uint32_t)digest[1] << 8) |
           ((uint32_t)digest[2] << 16) | ((uint32_t)digest[3] << 24);
    return true;
}

bool LoRaWAN_Crypto_EncryptPayload(const uint8_t *key, const uint8_t *input,
                                   uint8_t length, uint32_t devAddr, uint32_t fCnt,
                                   uint8_t direction, uint8_t *output)
{
    if (key == NULL || input == NULL || output == NULL)
    {
        return false;
    }

    aes_context ctx;
    if (aes_set_key(key, 16, &ctx) != 0)
    {
        return false;
    }

    uint8_t a[LORAWAN_BLOCK_SIZE];
    uint8_t s[LORAWAN_BLOCK_SIZE];
    uint16_t counter = 1;
    uint8_t blockCount = (length + LORAWAN_BLOCK_SIZE - 1) / LORAWAN_BLOCK_SIZE;

    for (uint8_t i = 0; i < blockCount; i++)
    {
        PrepareAi(a, direction, devAddr, fCnt, counter++);
        if (aes_encrypt(a, s, &ctx) != 0)
        {
            return false;
        }

        uint8_t blockSize = (length >= LORAWAN_BLOCK_SIZE) ? LORAWAN_BLOCK_SIZE : length;
        for (uint8_t j = 0; j < blockSize; j++)
        {
            output[i * LORAWAN_BLOCK_SIZE + j] = input[i * LORAWAN_BLOCK_SIZE + j] ^ s[j];
        }
        length -= blockSize;
    }

    return true;
}
