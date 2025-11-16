# Golden Uplink: Calibration ACK (opcode 0xA0)

## Payload
```
[0] opcode echo (0xA0)
[1] status (0 = OK, 1 = invalid)
[2] crc_hi
[3] crc_lo
```

## Expected Example
```
0xA0 0x00 0x12 0x4F
```

## Rules
- MUST be uplinked immediately after successful calibration  
- MUST reflect NVMM CRC of new calibration block  
