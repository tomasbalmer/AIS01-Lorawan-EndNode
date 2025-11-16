#ifndef UPLINK_ENCODER_H
#define UPLINK_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor.h"
#include "mac_mirror.h"

typedef struct
{
    uint8_t *buffer;
    uint8_t maxSize;
    uint8_t size;
} UplinkPayload_t;

typedef struct
{
    uint8_t adrEnabled;
    uint8_t dataRate;
    uint8_t txPower;
    uint8_t freqBand;
    int16_t rssi;
    int8_t snr;
    uint32_t frameCounterUp;
    uint8_t batteryLevel;
} UplinkStatusContext_t;

bool UplinkEncoder_EncodeStatus(const UplinkStatusContext_t *ctx, UplinkPayload_t *out);
/*
 * STATUS FRAME FORMAT
 * Byte 0 : 0x01
 * Byte 1 : batteryLevel
 * Byte 2 : adrEnabled
 * Byte 3 : dataRate
 * Byte 4 : txPower
 * Byte 5 : freqBand
 * Byte 6 : rssi (int8)
 * Byte 7 : snr  (int8)
 * Byte 8-11 : frameCounterUp (little-endian uint32)
 */
bool UplinkEncoder_EncodeCalibration(const uint8_t *calData, uint8_t calSize, UplinkPayload_t *out);
/*
 * CALIBRATION FRAME FORMAT
 * Byte 0 : 0xA0
 * Byte 1..N : raw calibration payload
 */
bool UplinkEncoder_EncodeDebug(uint8_t fwMajor,
                               uint8_t fwMinor,
                               uint8_t fwPatch,
                               uint8_t loraState,
                               UplinkPayload_t *out);
/*
 * DEBUG FRAME FORMAT
 * Byte 0 : 0xF0
 * Byte 1 : FW major
 * Byte 2 : FW minor
 * Byte 3 : FW patch
 * Byte 4 : LoRaWAN state
 * Byte 5 : 0x00 (reserved)
 */
bool UplinkEncoder_EncodeSensorStub(UplinkPayload_t *out);
/*
 * SENSOR FRAME STUB (placeholder until Phase 5)
 * Always returns false unless build is in DEBUG_STUB mode.
 */
bool UplinkEncoder_EncodeSensorFrame(const SensorSample_t *sample,
                                     uint8_t batteryLevel,
                                     UplinkPayload_t *out);
/*
 * SENSOR FRAME FORMAT (OEM FORMAT)
 * Byte 0  : 0x02
 * Byte 1  : battery %
 * Byte 2  : reserved = 0x00
 * Byte 3  : reserved = 0x00
 * Byte 4-5: primary sensor value (uint16 LE)
 * Byte 6-7: secondary sensor value (uint16 LE)
 * Byte 8-11: timestampMs (uint32 LE)
 * Total = 12 bytes
 */

/*
 * SENSOR-STATS CONTEXT
 * (Stub until Phase 8 introduces real collection routines)
 */
typedef struct
{
    uint8_t batteryLevel;       /* % */
    uint16_t primaryMin;
    uint16_t primaryAvg;
    uint16_t primaryMax;
    uint16_t secondaryMin;
    uint16_t secondaryAvg;
    uint16_t secondaryMax;
    uint8_t motionState;        /* 0/1 */
    uint8_t occupancyState;     /* 0/1 */
    uint8_t sensorMode;         /* enum mapped to uint8 */
    uint32_t timestampMs;
} UplinkSensorStatsContext_t;

/*
 * SENSOR-STATS FRAME FORMAT (OEM APPROXIMATION)
 * Byte 0  : 0x03
 * Byte 1  : battery %
 * Byte 2  : motion
 * Byte 3  : occupancy
 * Byte 4  : mode
 * Byte 5-6: primaryMin
 * Byte 7-8: primaryAvg
 * Byte 9-10: primaryMax
 * Byte 11-12: secondaryMin
 * Byte 13-14: secondaryAvg
 * Byte 15-16: secondaryMax
 * Byte 17-20: timestampMs
 * Total = 21 bytes
 */
bool UplinkEncoder_EncodeSensorStats(const UplinkSensorStatsContext_t *ctx,
                                     UplinkPayload_t *out);

/*
 * EXPANDED STATUS FRAME CONTEXT
 */
typedef struct
{
    uint8_t adrEnabled;
    uint8_t dataRate;
    uint8_t txPower;
    uint8_t freqBand;
    int16_t rssi;
    int8_t snr;
    uint8_t batteryLevel;
    uint16_t batteryMv;
    uint8_t sensorPowered;
    uint8_t sensorMode;
    uint32_t uptimeSec;
    uint32_t frameCounterUp;
    uint8_t pendingDl;
} UplinkStatusExContext_t;

/*
 * EXPANDED STATUS FRAME FORMAT (OEM FORMAT F1)
 * Byte 0  : 0xF1
 * Byte 1  : battery %
 * Byte 2  : ADR enabled
 * Byte 3  : DR
 * Byte 4  : TX power
 * Byte 5  : freqBand
 * Byte 6-7: battery mV (LE)
 * Byte 8  : sensor powered
 * Byte 9  : sensor mode
 * Byte 10 : pending DL
 * Byte 11 : RSSI (int8)
 * Byte 12 : SNR (int8)
 * Byte 13-16: uptime (uint32 LE)
 * Byte 17-20: FCntUp
 * Total = 21 bytes
 */
bool UplinkEncoder_EncodeStatusEx(const UplinkStatusExContext_t *ctx,
                                  UplinkPayload_t *out);

typedef struct
{
    uint8_t payload[MAC_MIRROR_MAX_SIZE];
    uint8_t size;
} UplinkMacMirrorContext_t;

/*
 * MAC-MIRROR FRAME FORMAT:
 * Byte 0  : 0xF2
 * Byte 1..N : raw MAC command bytes
 */
bool UplinkEncoder_EncodeMacMirror(const UplinkMacMirrorContext_t *ctx,
                                   UplinkPayload_t *out);

#endif /* UPLINK_ENCODER_H */
