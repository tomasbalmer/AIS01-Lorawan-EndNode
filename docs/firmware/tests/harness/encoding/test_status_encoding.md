# Encoding Test — Status Frame

## Purpose
Validate the frame used for periodic status reporting.

## Inputs
```
status_flags = 0x03
battery_mv   = 3730
uptime_s     = 157
```

## Expected Output
```
03 0E 92 00 00 9D 00 00
```

## Rules
- Must always include battery + uptime  
- Flags must match bitmask definitions  
- Length MUST match frame spec  
