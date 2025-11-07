#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SENSOR_MODE_LOW_POWER = 0,
    SENSOR_MODE_NORMAL = 1,
    SENSOR_MODE_HIGH_PRECISION = 2
} SensorMode_t;

typedef struct
{
    uint16_t primary;
    uint16_t secondary;
    uint32_t timestampMs;
    bool valid;
} SensorSample_t;

typedef struct
{
    uint32_t parameter;
    uint32_t value;
    uint32_t applyCount;
} SensorCalibrationState_t;

bool Sensor_Init(void);
void Sensor_Deinit(void);
bool Sensor_IsInitialized(void);
bool Sensor_SetPower(bool enable);
bool Sensor_IsPowered(void);
bool Sensor_SetMode(SensorMode_t mode);
SensorMode_t Sensor_GetMode(void);
bool Sensor_Read(SensorSample_t *sample);
bool Sensor_GetLastSample(SensorSample_t *sample);
void Sensor_ResetCalibration(void);
bool Sensor_UpdateCalibration(uint32_t parameter, uint32_t value);
const SensorCalibrationState_t *Sensor_GetCalibration(void);
void Sensor_Process(void);
bool Sensor_GetFrame(uint8_t *buffer, uint16_t maxLen, uint16_t *actualLen);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_H */
