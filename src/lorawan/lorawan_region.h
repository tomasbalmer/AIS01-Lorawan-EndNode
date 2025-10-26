#ifndef LORAWAN_REGION_H
#define LORAWAN_REGION_H

#include <stdint.h>
#include <stdbool.h>
#include "lorawan_types.h"

typedef struct
{
    uint32_t Frequency;
    uint8_t DrMin;
    uint8_t DrMax;
} LoRaWANChannel_t;

typedef struct
{
    const LoRaWANChannel_t *Channels;
    uint8_t ChannelCount;
    uint32_t Rx2Frequency;
    uint8_t Rx2DataRate;
    uint8_t MaxEirp;
    uint8_t NbJoinTrials;
} LoRaWANRegionParams_t;

const LoRaWANRegionParams_t *LoRaWAN_RegionGetParams(LoRaWANRegion_t region);
bool LoRaWAN_RegionValidateDr(LoRaWANRegion_t region, uint8_t dr);
bool LoRaWAN_RegionValidateTxPower(LoRaWANRegion_t region, uint8_t txPower);
uint32_t LoRaWAN_RegionGetJoinFrequency(LoRaWANRegion_t region, uint8_t attempt);
uint32_t LoRaWAN_RegionGetUplinkFrequency(LoRaWANRegion_t region, uint8_t channel);
uint8_t LoRaWAN_RegionGetNextChannel(LoRaWANRegion_t region, uint32_t *timeToNext);

#endif /* LORAWAN_REGION_H */
