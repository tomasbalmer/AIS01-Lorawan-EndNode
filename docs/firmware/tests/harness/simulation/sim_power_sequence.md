# Simulation — Power Sequence (STOP → WAKE)

## STOP Input State
```
rtc_alarm_set = true
sx1276 = off
clocks = disabled
iwdg = active
```

## Expected STOP Behavior
- Current < 20 µA
- No peripheral activity
- No spontaneous wake

## Expected WAKE Sequence
1. Clocks enabled  
2. GPIO restored  
3. SX1276 regulator ON  
4. SX1276 init  
5. LoRaMAC state restored  
6. scheduler resumes  

## Fail Conditions
- WAKE without RTC
- high STOP current
- radio not reinitialized

---
