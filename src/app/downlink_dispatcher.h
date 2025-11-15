#ifndef DOWNLINK_DISPATCHER_H
#define DOWNLINK_DISPATCHER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DOWNLINK_RESULT_OK = 0,
    DOWNLINK_RESULT_INVALID_OPCODE,
    DOWNLINK_RESULT_INVALID_LENGTH,
    DOWNLINK_RESULT_ERROR
} DownlinkResult_t;

typedef struct {
    uint8_t opcode;
    uint8_t length;
    int16_t rssi;
    int8_t snr;
} DownlinkContext_t;

typedef struct {
    void (*setTdc)(uint32_t intervalMs);
    void (*setAdr)(bool enabled);
    void (*setDataRate)(uint8_t dataRate);
    void (*setTxPower)(uint8_t power);
    bool (*processCalibration)(const uint8_t *payload, uint8_t size);
} DownlinkActions_t;

DownlinkResult_t Downlink_Handle(
    const uint8_t *payload,
    uint8_t size,
    const DownlinkContext_t *ctx,
    const DownlinkActions_t *actions
);

#endif /* DOWNLINK_DISPATCHER_H */
