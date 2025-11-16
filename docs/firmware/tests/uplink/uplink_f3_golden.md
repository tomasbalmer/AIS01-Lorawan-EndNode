# Golden Uplink: F3 Power Profile

## Frame Purpose
OEM-style power profile including:
- battery percent  
- battery millivolts  
- uptime seconds  
- sensor power flag  
- DR

## Structure (10 bytes)
```
[0] battery_percent   (uint8)
[1] battery_mv_hi     (uint8)
[2] battery_mv_lo     (uint8)
[3] uptime_hi         (uint8)
[4] uptime_md         (uint8)
[5] uptime_lo         (uint8)
[6] sensor_pwr_flag   (uint8)
[7] datarate          (uint8)
[8] reserved0         (uint8)
[9] reserved1         (uint8)
```

## Expected Example (test vector)
```
battery = 3730 mV
percent = 92%
uptime = 157 seconds
DR = 2
sensor_pwr = 1
```

Expected bytes:
```
0x5C 0x0E 0x92 0x00 0x00 0x9D 0x01 0x02 0x00 0x00
```

## Validation Rules
- Length MUST be exactly 10 bytes  
- DR must match MAC state  
- battery MUST be raw ADC → mV conversion
- uptime MUST match systime seconds
