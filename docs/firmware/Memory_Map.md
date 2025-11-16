# Firmware Memory Map (Source of Truth)

## Bootloader (OEM)
Start: 0x08000000  
End:   0x0800EFFF  
Notes: Verified via OEM Bootloader v1.4 binary.

## Application
Start: 0x0800F000  
End:   End of flash  
Vector Table: 0x0800F000  
Notes: Matches OEM App Reset_Handler at 0x0800F30D.

## Rule
This file overrides all previous documentation.
All diagrams, specifications, and references **must** follow this map.
