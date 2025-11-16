# Encoding Test — Calibration ACK

## Purpose
Validate the ACK uplink after opcode 0xA0 calibration update.

## Inputs
```
crc = 0x124F
status = 0
```

## Expected Output
```
A0 00 12 4F
```

## Rules
- A0 must be echoed  
- status = 0 (OK) or 1 (error)  
- crc_hi, crc_lo derived from NVMM  

## Pass Criteria
- byte-for-byte equality  
