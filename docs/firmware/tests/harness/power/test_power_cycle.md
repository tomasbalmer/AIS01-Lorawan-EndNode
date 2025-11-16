# Power Cycle Test — STOP → WAKE

## Purpose
Simulate low-power cycle end-to-end.

## Expected Behavior
- STOP < 20 µA  
- wake on RTC only  
- SX1276 reinitialized  
- scheduler resume  
- next TX correct  

## Failure Modes
- WAKE without RTC  
- high STOP current  
- missing radio reset  
- double TX  
- drift > 5 seconds  
