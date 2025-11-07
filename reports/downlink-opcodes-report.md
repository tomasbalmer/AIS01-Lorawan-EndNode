# Agent A Report — Downlink Opcode Table (2025‑11‑07)

**Source:** Agent A  
**Requested by:** Agent B  

## Table Snapshot
- **Firmware:** AIS01-LB AU915 (1).bin  
- **Address Range:** `0x08014ABC–0x08014B40`  
- **Entries:** 33 (header constants + handler references)

| Index | Raw Value | Target | Type | Notes |
|------:|-----------|--------|------|-------|
| 0 | `0x0800D125` | – | Data | Init stub |
| 1 | `0x0800D101` | – | Data | Secondary init |
| 2 | `0x00000007` | – | Const | Flag value |
| 3 | `0x20004B08` | RAM | Data | JPEG buffer base |
| 4 | `0x00000000` | – | Null | Terminator |
| 5 | `0x000493E0` | – | Const | JPEG size limit (~300 kB) |
| 6 | `0x00000001` | – | Const | Enable flag |
| 7 | `0x20006D7C` | RAM | Data | Sensor metadata buffer |
| 8 | `0xC8010001` | – | Const | Status/CRC word |
| 9 | `0x080121A9` | `FUN_08012118` | Code | memset-style helper |
|10 | `0x08011FE1` | `opcode_handler_flag_and_timer` | Code | Sets flag 0x02, starts timer `DAT_080120B8` |
|11 | `0x080121E1` | `opcode_validator_memcmp` | Code | memcmp validator |
|12 | `0x0801220F` | – | Data | Placeholder |
|13 | `0x08013F91` | – | Data | String “Delay between …” |
|14 | `0x080123E1` | `opcode_parse_uint` | Code | ASCII integer parser |
|15 | `0x080120B5` | – | Data | Constant `0x57` (‘W’) |
|16 | `0x00000014` | – | Const | Value 20 |
|17 | `0x00000000` | – | Null | Gap |
|18 | `0x00006655` | – | Const | Flag pattern |
|19 | `0x01010001` | – | Const | Pattern word |
|20 | `0x01010101` | – | Const | Fill |
|21 | `0x00000101` | – | Const | Marker |
|22–32 | `0x01010101` | – | Const | Padding |

## Function Summaries
- **`FUN_08012118`**: Optimised memset used to clear sensor/JPEG buffers before processing.  
- **`opcode_handler_flag_and_timer` (0x08011FE1)**: Writes `0x02` to context byte `ctx[3]`, calls `FUN_0800D768(DAT_080120C0)`, and starts timer at `DAT_080120B8` if the runtime handle is ready.  
- **`opcode_validator_memcmp` (0x080121E1)**: Straightforward memcmp-style validator to ensure payload mirrors stored context before executing actions.  
- **`opcode_parse_uint` (0x080123E1)**: Parses ASCII integers (+/- signs, bases 8/10/16) using lookup table `DAT_080123DC`.

## Referenced Buffers & Constants
- `0x20004B08`: Primary JPEG buffer (shared with AT+CALIBREMOTE / sensor bridge).  
- `0x20006D7C`: Metadata/work buffer (frame flags, CRC).  
- `DAT_080120B8`: Timer handle for sensor scheduling.  
- `DAT_080120C0`: Context passed to `FUN_0800D768` prior to setting the timer.  
- `0x08013F91`: User string “Delay between the end of the Tx …” reused for error reporting.

## Notes
- The first nine entries define structural data for the dispatcher (limits, buffer pointers, status flags).  
- Executable handlers occupy indices 9–14 and map to the helper functions described above; opcodes higher than that reuse shared constants.  
- Padding values (`0x01010101`) suggest unused slots reserved for future handlers.
