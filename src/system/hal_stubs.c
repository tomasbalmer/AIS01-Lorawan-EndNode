#include "system/hal_stubs.h"
#include "system/delay.h"
#include "board/rtc-board.h"

uint32_t HAL_GetTick(void)
{
    return RtcGetTimerValue();
}

void HAL_Delay(uint32_t delay)
{
    DelayMs(delay);
}
