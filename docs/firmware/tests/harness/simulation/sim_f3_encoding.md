# Simulation — F3 Encoding

## Input (simulation)
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

## Pass Criteria
- EXACT byte equality
- No padding bytes
- No endian reversal errors

---
