#include "hal_stubs.h"
#include "delay.h"
#include "rtc-board.h"

uint32_t HAL_GetTick(void)
{
    return RtcGetTimerValue();
}

void HAL_Delay(uint32_t delay)
{
    DelayMs(delay);
}
