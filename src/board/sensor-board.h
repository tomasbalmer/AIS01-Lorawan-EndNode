#ifndef SENSOR_BOARD_H
#define SENSOR_BOARD_H

#include <stdbool.h>
#include <stdint.h>

bool Sensor_BoardConfigure(void);
bool Sensor_BoardPowerControl(bool enable);
bool Sensor_BoardAcquire(uint16_t *primary, uint16_t *secondary);
bool Sensor_HwFetchFrame(uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen);
bool Sensor_HwPerformHandshake(void);
void Sensor_HwFlushRx(void);
bool Sensor_HwReady(void);

#endif /* SENSOR_BOARD_H */
