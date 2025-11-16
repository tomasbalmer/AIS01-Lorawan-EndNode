# Simulation — NVMM CRC Recompute

## Input
```
previous_crc = <old>
calibration_payload = 12 34 56 78
version = 1
```

## Expected Behavior
1. Build NVMM block  
2. Compute CRC32 over (header + version + payload)  
3. CRC32 changes  
4. Old CRC invalidated  
5. New CRC stored  

## Pass:
- computed CRC matches golden formula
- length of CRC region matches NVMM spec
