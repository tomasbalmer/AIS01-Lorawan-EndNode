#include "downlink_dispatcher.h"

#include <stddef.h>
#include <string.h>

typedef bool (*DownlinkValidator_t)(const uint8_t *payload, uint8_t size);
typedef DownlinkResult_t (*DownlinkHandler_t)(const uint8_t *payload, uint8_t size,
                                              const DownlinkContext_t *ctx,
                                              const DownlinkActions_t *actions);

typedef struct
{
    uint8_t opcode;
    DownlinkValidator_t validate;
    DownlinkHandler_t handler;
} DownlinkCommand_t;

static bool ValidateSetTdc(const uint8_t *payload, uint8_t size)
{
    (void)payload;
    return size == 5U;
}

static bool ValidateAdr(const uint8_t *payload, uint8_t size)
{
    (void)payload;
    return size >= 2U;
}

static bool ValidateDataRate(const uint8_t *payload, uint8_t size)
{
    (void)payload;
    return size >= 2U;
}

static bool ValidateTxPower(const uint8_t *payload, uint8_t size)
{
    (void)payload;
    return size >= 2U;
}

static bool ValidateCalibration(const uint8_t *payload, uint8_t size)
{
    (void)payload;
    return size >= 1U;
}

static DownlinkResult_t HandleSetTdc(const uint8_t *payload, uint8_t size,
                                     const DownlinkContext_t *ctx,
                                     const DownlinkActions_t *actions)
{
    (void)size;
    (void)ctx;

    if ((actions == NULL) || (actions->setTdc == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    uint32_t interval = 0;
    memcpy(&interval, &payload[1], sizeof(uint32_t));
    actions->setTdc(interval);
    return DOWNLINK_RESULT_OK;
}

static DownlinkResult_t HandleAdr(const uint8_t *payload, uint8_t size,
                                  const DownlinkContext_t *ctx,
                                  const DownlinkActions_t *actions)
{
    (void)size;
    (void)ctx;

    if ((actions == NULL) || (actions->setAdr == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    actions->setAdr(payload[1] != 0U);
    return DOWNLINK_RESULT_OK;
}

static DownlinkResult_t HandleDataRate(const uint8_t *payload, uint8_t size,
                                       const DownlinkContext_t *ctx,
                                       const DownlinkActions_t *actions)
{
    (void)size;
    (void)ctx;

    if ((actions == NULL) || (actions->setDataRate == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    actions->setDataRate(payload[1]);
    return DOWNLINK_RESULT_OK;
}

static DownlinkResult_t HandleTxPower(const uint8_t *payload, uint8_t size,
                                      const DownlinkContext_t *ctx,
                                      const DownlinkActions_t *actions)
{
    (void)size;
    (void)ctx;

    if ((actions == NULL) || (actions->setTxPower == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    actions->setTxPower(payload[1]);
    return DOWNLINK_RESULT_OK;
}

static DownlinkResult_t HandleCalibration(const uint8_t *payload, uint8_t size,
                                          const DownlinkContext_t *ctx,
                                          const DownlinkActions_t *actions)
{
    (void)ctx;

    if ((actions == NULL) || (actions->processCalibration == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    if (!actions->processCalibration(payload, size))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    return DOWNLINK_RESULT_OK;
}

static const DownlinkCommand_t g_DownlinkCommands[] = {
    {0x01U, ValidateSetTdc, HandleSetTdc},
    {0x21U, ValidateAdr, HandleAdr},
    {0x22U, ValidateDataRate, HandleDataRate},
    {0x23U, ValidateTxPower, HandleTxPower},
    {0xA0U, ValidateCalibration, HandleCalibration},
};

static const DownlinkCommand_t *FindCommand(uint8_t opcode)
{
    for (size_t i = 0; i < (sizeof(g_DownlinkCommands) / sizeof(g_DownlinkCommands[0])); ++i)
    {
        if (g_DownlinkCommands[i].opcode == opcode)
        {
            return &g_DownlinkCommands[i];
        }
    }
    return NULL;
}

DownlinkResult_t Downlink_Handle(
    const uint8_t *payload,
    uint8_t size,
    const DownlinkContext_t *ctx,
    const DownlinkActions_t *actions)
{
    if ((payload == NULL) || (ctx == NULL) || (actions == NULL))
    {
        return DOWNLINK_RESULT_ERROR;
    }

    if (size == 0U)
    {
        return DOWNLINK_RESULT_INVALID_LENGTH;
    }

    const DownlinkCommand_t *command = FindCommand(ctx->opcode);
    if (command == NULL)
    {
        return DOWNLINK_RESULT_INVALID_OPCODE;
    }

    if (!command->validate(payload, size))
    {
        return DOWNLINK_RESULT_INVALID_LENGTH;
    }

    return command->handler(payload, size, ctx, actions);
}
