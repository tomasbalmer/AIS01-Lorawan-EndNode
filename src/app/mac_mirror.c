#include "mac_mirror.h"
#include <string.h>

static MacMirrorFrame_t g_LastMacCmd = { { 0 }, 0U };

void MacMirror_StoreRx(const uint8_t *data, uint8_t size)
{
    if (!data || size == 0U)
    {
        return;
    }
    if (size > MAC_MIRROR_MAX_SIZE)
    {
        size = MAC_MIRROR_MAX_SIZE;
    }
    memcpy(g_LastMacCmd.buffer, data, size);
    g_LastMacCmd.size = size;
}

bool MacMirror_GetLast(MacMirrorFrame_t *out)
{
    if (!out || g_LastMacCmd.size == 0U)
    {
        return false;
    }
    memcpy(out->buffer, g_LastMacCmd.buffer, g_LastMacCmd.size);
    out->size = g_LastMacCmd.size;
    return true;
}

void MacMirror_Clear(void)
{
    g_LastMacCmd.size = 0U;
}
