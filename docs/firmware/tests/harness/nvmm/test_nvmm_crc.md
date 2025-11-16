# NVMM Test — CRC Consistency

## Purpose
Validate CRC32 correctness for the NVMM block.

## Inputs
```
calibration_payload = <example bytes>
version = 1
```

## Expected Behavior
1. Build NVMM block (header + version + payload + CRC)
2. Compute CRC32 over data without CRC field
3. CRC32 must match expected golden value

## Failure Modes
- wrong byte ordering  
- CRC over too large region  
- missing header fields  
- uninitialized bytes  
