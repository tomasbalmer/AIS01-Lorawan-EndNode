#ifndef MAC_MIRROR_H
#define MAC_MIRROR_H

#include <stdint.h>
#include <stdbool.h>

#define MAC_MIRROR_MAX_SIZE 16

typedef struct
{
    uint8_t buffer[MAC_MIRROR_MAX_SIZE];
    uint8_t size;
} MacMirrorFrame_t;

/*!
 * \brief Store last received MAC command from network
 */
void MacMirror_StoreRx(const uint8_t *data, uint8_t size);

/*!
 * \brief Retrieve last received MAC command
 */
bool MacMirror_GetLast(MacMirrorFrame_t *out);

/*!
 * \brief Clear stored MAC command
 */
void MacMirror_Clear(void);

#endif /* MAC_MIRROR_H */
