# Encoding Test — F3 Power Profile

## Purpose
Validate the construction of the 10-byte F3 uplink payload.

## Inputs
```
battery_mv = 3730
uptime_s   = 157
sensor_pwr = 1
datarate   = 2
```

## Expected Output
```
5C 0E 92 00 00 9D 01 02 00 00
```

## Rules
- battery_mV split into high/low bytes  
- uptime encoded as 24-bit or 32-bit depending on source implementation  
- reserved bytes must be zero  
- length MUST be exactly 10 bytes  

## Pass Criteria
- byte-for-byte equality  
