#include <stddef.h>
#include "lorawan_region.h"
#include "lorawan_region_au915.h"

const LoRaWANRegionParams_t *LoRaWAN_RegionGetParams(LoRaWANRegion_t region)
{
    switch (region)
    {
    case LORAWAN_REGION_AU915:
    default:
        return LoRaWAN_RegionAU915();
    }
}

bool LoRaWAN_RegionValidateDr(LoRaWANRegion_t region, uint8_t dr)
{
    (void)region;
    return dr <= 13; /* DR0..DR13 defined for AU915, restrict later */
}

bool LoRaWAN_RegionValidateTxPower(LoRaWANRegion_t region, uint8_t txPower)
{
    (void)region;
    return txPower <= 10;
}

uint32_t LoRaWAN_RegionGetJoinFrequency(LoRaWANRegion_t region, uint8_t attempt)
{
    const LoRaWANRegionParams_t *params = LoRaWAN_RegionGetParams(region);
    if (params == NULL || params->ChannelCount == 0)
    {
        return 0;
    }
    uint8_t index = attempt % params->ChannelCount;
    return params->Channels[index].Frequency;
}

uint32_t LoRaWAN_RegionGetUplinkFrequency(LoRaWANRegion_t region, uint8_t channel)
{
    const LoRaWANRegionParams_t *params = LoRaWAN_RegionGetParams(region);
    if (params == NULL || channel >= params->ChannelCount)
    {
        return 0;
    }
    return params->Channels[channel].Frequency;
}

uint8_t LoRaWAN_RegionGetNextChannel(LoRaWANRegion_t region, uint32_t *timeToNext)
{
    (void)timeToNext;
    const LoRaWANRegionParams_t *params = LoRaWAN_RegionGetParams(region);
    if (params == NULL)
    {
        return 0;
    }
    static uint8_t s_NextChannel = 0;
    uint8_t channel = s_NextChannel % params->ChannelCount;
    s_NextChannel = (s_NextChannel + 1) % params->ChannelCount;
    return channel;
}
