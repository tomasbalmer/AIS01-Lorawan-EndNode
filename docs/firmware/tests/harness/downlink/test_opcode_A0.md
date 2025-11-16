# Downlink Test Harness — A0 Calibration

## Purpose
Simulation-driven test for calibration opcode.

## Steps
1. provide payload A0 …  
2. call dispatcher simulation  
3. verify:
   - validator ok  
   - calibration engine applied  
   - NVMM written  
   - ACK scheduled  
4. simulate next uplink and verify ACK matches expected bytes  

## Inputs
```
A0 01 12 34 56 78
```

## Expected ACK
```
A0 00 <crc_hi> <crc_lo>
```
