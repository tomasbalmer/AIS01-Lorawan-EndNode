#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t Crc32Init(void);
uint32_t Crc32Update(uint32_t crc, const void *buffer, size_t length);
uint32_t Crc32Finalize(uint32_t crc);

#endif /* CRC32_H */
