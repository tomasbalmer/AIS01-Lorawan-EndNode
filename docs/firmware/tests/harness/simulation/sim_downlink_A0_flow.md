# Simulation — Downlink A0 Calibration

## Input Payload
```
A0 01 12 34 56 78
```

## Expected Internal Behavior
1. Parsing OK  
2. Validator OK  
3. Calibration engine updates RAM  
4. NVMM block rebuilt  
5. CRC32 recomputed  
6. ACK scheduled

## Expected NVMM Block (example)
```
version: 1
payload: 12 34 56 78
crc32:   <computed>
```

## Expected ACK (uplink)
```
A0 00 <crc_hi> <crc_lo>
```

## Pass/Fail Conditions
- NVMM MUST change
- ACK MUST be scheduled
- validator errors MUST stop NVMM writes

---
