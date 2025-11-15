#ifndef UPLINK_ENCODER_H
#define UPLINK_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor.h"

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

#endif /* UPLINK_ENCODER_H */
