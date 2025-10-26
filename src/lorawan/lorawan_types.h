#ifndef LORAWAN_TYPES_H
#define LORAWAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define LORAWAN_DEV_NONCE_HISTORY_SIZE 8
#define LORAWAN_MAX_FOPTS_LEN          15
#define LORAWAN_MAX_PAYLOAD_LEN        242

typedef enum
{
    LORAWAN_DEVICE_CLASS_A = 0,
} LoRaWANDeviceClass_t;

typedef enum
{
    LORAWAN_REGION_AU915 = 0,
} LoRaWANRegion_t;

typedef enum
{
    LORAWAN_STATUS_SUCCESS = 0,
    LORAWAN_STATUS_ERROR,
    LORAWAN_STATUS_BUSY,
    LORAWAN_STATUS_NOT_JOINED,
    LORAWAN_STATUS_INVALID_PARAM,
} LoRaWANStatus_t;

typedef enum
{
    LORAWAN_ADR_OFF = 0,
    LORAWAN_ADR_ON,
} LoRaWANAdrState_t;

typedef enum
{
    LORAWAN_MSG_UNCONFIRMED = 0,
    LORAWAN_MSG_CONFIRMED,
} LoRaWANMsgType_t;

typedef struct
{
    uint8_t DevEui[8];
    uint8_t AppEui[8];
    uint8_t AppKey[16];
    uint8_t JoinNonceHistory[LORAWAN_DEV_NONCE_HISTORY_SIZE][3];
    uint32_t DevNonceCounter;
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
    uint32_t DevAddr;
    uint32_t FCntUp;
    uint32_t FCntDown;
    bool Joined;
} LoRaWANSession_t;

typedef struct
{
    LoRaWANRegion_t Region;
    LoRaWANDeviceClass_t DeviceClass;
    LoRaWANAdrState_t AdrState;
    uint8_t DataRate;
    uint8_t TxPower;
    uint8_t Rx2DataRate;
    uint32_t Rx2Frequency;
    uint8_t SubBand;
    LoRaWANMsgType_t MsgType;
    uint8_t AppPort;
    uint32_t TxDutyCycleMs;
} LoRaWANSettings_t;

typedef struct
{
    uint8_t *Buffer;
    uint8_t BufferLen;
    uint8_t Port;
    LoRaWANMsgType_t MsgType;
} LoRaWANFrame_t;

#endif /* LORAWAN_TYPES_H */
