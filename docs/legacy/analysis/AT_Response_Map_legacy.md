# AT Response / Message Map (AIS01-LB)

## Success Responses
| String | Address | Usage |
|--------|---------|-------|
| `"\r\nOK\r\n"` | `0x08013222` | Generic success for AT commands, including calibration apply acknowledgements |
| `"Correct Password\r\n"` | `0x0801810B` | Emitted after matching AT password gate |
| `"JOINED\n\r"` | `0x08017322` | Printed when LoRaWAN join succeeds |

## Error Responses
| String | Address | Context |
|--------|---------|---------|
| `"AT_ERROR"` | `0x0801814D` | Generic failure response |
| `"AT_PARAM_ERROR"` | `0x08018130` | Parameter validation failure |
| `"AT_BUSY_ERROR"` | `0x0801811F` | Busy state during operations |
| `"write config error\r\n"` | `0x080180A8` | Storage write failure |
| `"erase all sensor data storage error\r\n"` | `0x080180C9` | Sensor data erase failure |

## Calibration / Configuration Messages
| String | Address | Meaning |
|--------|---------|---------|
| `"Set after calibration time or take effect after ATZ"` | `0x08017575` | Indicates delayed effect for calibration changes |
| `"Clear all stored sensor data..."` | `0x080180E8` | Triggered during calibration cleanup commands |
| `"Set sleep mode\r\n"` | `0x08017E9D` | Printed when entering sleep (related to calibration power states) |

## Device / System Messages
| String | Address | Usage |
|--------|---------|-------|
| `"AIS01_LB Detected\r\n"` | `0x08017366` | Device identification |
| `"Image Version: v1.0.5\n\r"` | `0x08017404` | Firmware version banner |
| `"LoRaWan Stack: DR-LWS-007\n\r"` | `0x080174DD` | LoRaWAN stack info |
| `"system_reset\r\n"` | `0x08018256` | Printed on reboot (ATZ) |

## Notes
- Addresses based on `AIS01_strings.csv` and Ghidra findings (`0x08013200–0x08014570`).
- Ensure rewritten firmware emits identical strings to maintain compatibility with Dragino tools and scripts.
