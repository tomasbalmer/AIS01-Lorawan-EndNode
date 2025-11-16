# Scheduler Rule Tests

## Purpose
Validate fundamental timing rules.

## Rules
- join must complete before TX  
- forced uplinks override TDC  
- calibration ACK shortens next deadline  
- STOP always occurs before TX (unless ACK pending)  
- must not TX twice without STOP  

## Fail Conditions
- double TX  
- STOP skipped  
- infinite join loop  
