# Golden Uplink: Status Frame

## Structure
```
status_flags   (uint8)
battery_mv     (uint16)
uptime_seconds (uint32)
```

## Example Result
```
0x03 0x0E 0x92 0x00 0x00 0x9D 0x00 0x00
```

## Notes
- status_flags bitmask depends on runtime state  
- MUST include battery + uptime always  
