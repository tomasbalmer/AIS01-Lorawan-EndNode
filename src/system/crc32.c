#include "crc32.h"

#define CRC32_POLY 0xEDB88320UL

uint32_t Crc32Init(void)
{
    return 0xFFFFFFFFUL;
}

uint32_t Crc32Update(uint32_t crc, const void *buffer, size_t length)
{
    const uint8_t *data = (const uint8_t *)buffer;
    for (size_t i = 0; i < length; ++i)
    {
        uint32_t byte = data[i];
        crc ^= byte;
        for (uint32_t j = 0; j < 8; ++j)
        {
            uint32_t mask = -(crc & 1UL);
            crc = (crc >> 1) ^ (CRC32_POLY & mask);
        }
    }
    return crc;
}

uint32_t Crc32Finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFUL;
}
