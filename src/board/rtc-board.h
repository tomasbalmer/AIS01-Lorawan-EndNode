#ifndef RTC_BOARD_H
#define RTC_BOARD_H

#include <stdint.h>
#include "timer.h"

void RtcInit(void);
uint32_t RtcSetTimerContext(void);
uint32_t RtcGetTimerContext(void);
uint32_t RtcGetMinimumTimeout(void);
uint32_t RtcMs2Tick(uint32_t milliseconds);
uint32_t RtcTick2Ms(uint32_t tick);
void RtcDelayMs(uint32_t delay);
void RtcSetAlarm(uint32_t timeout);
void RtcStopAlarm(void);
void RtcStartAlarm(uint32_t timeout);
uint32_t RtcGetTimerValue(void);
uint32_t RtcGetTimerElapsedTime(void);
uint32_t RtcGetCalendarTime(uint16_t *milliseconds);
void RtcBkupWrite(uint32_t data0, uint32_t data1);
void RtcBkupRead(uint32_t *data0, uint32_t *data1);
void RtcProcess(void);
TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature);
void RtcOnSysTick(void);

#endif /* RTC_BOARD_H */
