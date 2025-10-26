#include "lorawan_region_au915.h"

static const LoRaWANChannel_t s_Au915Channels[] = {
    { 915200000, 2, 5 },
    { 915400000, 2, 5 },
    { 915600000, 2, 5 },
    { 915800000, 2, 5 },
    { 916000000, 2, 5 },
    { 916200000, 2, 5 },
    { 916400000, 2, 5 },
    { 916600000, 2, 5 },
};

static const LoRaWANRegionParams_t s_Au915Params = {
    .Channels = s_Au915Channels,
    .ChannelCount = sizeof(s_Au915Channels) / sizeof(s_Au915Channels[0]),
    .Rx2Frequency = 923300000,
    .Rx2DataRate = 8,
    .MaxEirp = 20,
    .NbJoinTrials = 3,
};

const LoRaWANRegionParams_t *LoRaWAN_RegionAU915(void)
{
    return &s_Au915Params;
}
