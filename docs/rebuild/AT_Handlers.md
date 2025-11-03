# AT Command Layer

| Command | Handler Function | Address | Purpose | Cross-Refs |
|---------|------------------|---------|---------|------------|
| `AT+JOIN` | `FUN_00002030` | `0x08006030` | Triggers OTAA join via LoRa core (`0x0800522C`), validates join mode from `AT+NJM`. | `docs/AIS01_bin_analysis/AIS01_function_analysis.md` (index 405), `AIS01_strings.csv` (`+JOIN` at `0x08018014`) |
| `AT+TDC` | `FUN_0000203C` | `0x0800603C` | Sets transmission interval (sec), enforces ≥4s constraint (`0x0800642C`) before persisting. | `AIS01_function_analysis.md` (406, 490), `AIS01_strings.csv` (`TDC setting needs to be high than 4s` at `0x080175F9`) |
| `AT+ADR` | `FUN_00002048` | `0x08006048` | Enables/disables ADR and informs LoRa core ADR controller (`0x08005250`). | `AIS01_function_analysis.md` (407), `AIS01_strings.csv` (`Attention:Take effect after AT+ADR=0` at `0x08014277`) |
| `AT+DR` | `FUN_00002054` | `0x08006054` | Sets data rate, cross-checks AU915 limits via channel calculator (`0x08005274`). | `AIS01_function_analysis.md` (408), `AIS01_strings.csv` (`Get or Set the Data Rate` at `0x08017C3F`) |
| `AT+TXP` | `FUN_00002060` | `0x08006060` | Adjusts TX power and updates `FUN_00001250` plus NVM slot. | `AIS01_function_analysis.md` (409), `AIS01_strings.csv` (`0x08017CAA`) |
| `AT+CFG` | `FUN_0000206C` | `0x0800606C` | Dumps all persisted parameters by iterating NVM structure at `0x08007030`. | `AIS01_function_analysis.md` (410), `AIS01_strings.csv` (`Print all configurations` at `0x0801806E`) |
| `AT+VER` | `FUN_00002078` | `0x08006078` | Prints version banner (`LoRaWan Stack: DR-LWS-007`, `v1.0.5`). | `AIS01_function_analysis.md` (411), `AIS01_strings.csv` (`0x080174DD`) |
| `AT+FDR` | `FUN_00002084` | `0x08006084` | Factory reset: clears EEPROM via `0x0800706C`, reloads defaults, forces reboot. | `AIS01_function_analysis.md` (412, 510), `AIS01_strings.csv` (`Reset Parameters to Factory Default` at `0x0801760A`) |
| `AT+DEBUG` | `FUN_0000209C` | `0x0800609C` | Toggles verbose UART logs; gates debug prints in LoRa core and sensor stack. | `AIS01_function_analysis.md` (414), `AIS01_strings.csv` (`Use AT+DEBUG to see more debug info` at `0x080175C1`) |
| `AT+BAT` | `FUN_000020A8` | `0x080060A8` | Reads VBAT via ADC (handled by `0x08007054`), formats `Bat_voltage:%d mv`. | `AIS01_function_analysis.md` (415, 508), `AIS01_strings.csv` (`0x080174B8`) |
| `AT+PORT` | `FUN_000020B4` | `0x080060B4` | Sets uplink FPort used by `FUN_00001238`. | `AIS01_function_analysis.md` (416) |
| `AT+CALIBREMOTE` | *(inferred)* | `0x080062??` → `0x08007060` | Accepts hex payload, forwards to calibration engine (`0x08007060`) shared with downlink opcode `0xA0`. | `AIS01_overview.md` (Calibración remota), `AIS01_strings.csv` (`Set after calibration time...` at `0x08017575`) |
| `AT+TIMESTAMP` | `FUN_000021D4` | `0x080061D4` | Sets/gets UNIX timestamp, synchronises RTC via `0x08007024`. | `AIS01_function_analysis.md` (440), `AIS01_strings.csv` (`Set current timestamp=%u` at `0x08017641`) |
| `AT+SYNCMOD` | `FUN_000021EC` | `0x080061EC` | Chooses time sync source (`manual`, `LoRa DeviceTime`, `external`), influences scheduler. | `AIS01_function_analysis.md` (442), `AIS01_overview.md` (Temporal sync) |
| `AT+SLEEP` | `FUN_00002234` | `0x08006234` | Requests STOP-mode entry; delegates to `0x08007018` with parameters. | `AIS01_function_analysis.md` (448), `AIS01_strings.csv` (`SLEEP` at `0x08017380`) |
| `AT+GETSENSORVALUE` | `FUN_00002288` | `0x08006288` | Queries AI sensor over secondary UART handled by `0x0800703C`; prints last capture. | `AIS01_function_analysis.md` (455), `AIS01_strings.csv` (`AIS01_LB Detected` at `0x08017366`) |
| `AT+UPTM` | `FUN_00002360` | `0x08006360` | Forces immediate uplink, bypasses duty cycle, reuses LoRa send pipeline. | `AIS01_function_analysis.md` (473), `AIS01_strings.csv` (`Start Tx events` at `0x08017532`) |
| `AT+CHE` / `AT+CHS` | `FUN_00002180` / `FUN_00002174` | `0x08006180` / `0x08006174` | Select AU915 sub-band or channel mask; updates channel planner `0x08005274`. | `AIS01_function_analysis.md` (432-433), `AIS01_strings.csv` (`Error Subband` at `0x08017D6E`) |
| `AT+PORT`, `AT+PNACKMD`, `AT+RX2DR`, ... | See handlers `0x080060B4–0x080060FC` | `0x080060B4+` | Network parameter setters that call into LoRa core and storage. | `AIS01_function_analysis.md` (416-423) |

## Parser & Dispatch Flow
```
UART RX ISR → ring buffer
    ↓
FUN_00002000 (0x08006000)
    ├─ Tokenize command (upper-case, '=' or '?')
    ├─ Look up in table @0x08016A06 (name → handler → help)
    ├─ Enforce password gate (strings @0x0801810B)
    ├─ Validate parameters (shared validators @0x0800642C et al.)
    └─ Call handler (0x0800600C…0x08006474)
           ↓
       Optional persistence via storage (0x08007030)
           ↓
       Response emitted (`OK`, `AT_PARAM_ERROR`, etc.)
```

## Relationships to Other Firmware Blocks
- **LoRaWAN Core:** Most setters (ADR/DR/TXP/TDC/PORT) immediately queue updates through `FUN_00001250`, `FUN_00001238`, or `FUN_00001274`. `AT+JOIN` is the user-facing entry to the join routine.
- **Storage Module:** `AT+CFG`, `AT+FDR`, key setters (`DEVEUI`, `APPKEY`, `NWKSKEY`, etc.) rely on the flash manager at `0x08007030` and on validation helpers in the 0x080064xx range (`write key error`, `invalid credentials`).
- **Hardware & Power:** `AT+BAT`, `AT+SLEEP`, `AT+5VT`, `AT+INTMODx` call directly into GPIO/ADC/RTC wrappers (`0x08007018`, `0x08007054`, `0x08007024`). `AT+GETSENSORVALUE`, `AT+JPEGSIZE` couple to the sensor block at `0x0800703C/48`.
- **Downlink Symmetry:** Each downlink opcode (`0x01` TDC, `0x21` ADR, `0xA0` calibration) mirrors the AT handler logic, sharing validators and storage commits to keep UART and LoRa control paths aligned.

## How the AT Layer Fits in the Firmware Flow
The AT layer forms the primary human/device interface: during BOOT, `FUN_00002000` registers UART callbacks and loads the command table. While the application sits in IDLE or SLEEP, any UART wake-up routes through this parser; write operations first unlock via the password prompt, then forward to LoRa core or hardware managers. Responses reuse stock Dragino strings from `AIS01_strings.csv` to remain compatible with existing scripts and the Dragino OTA tool. Because handlers reuse the same validators as downlink commands, behaviour stays consistent whether configuration arrives locally or over the air.

## References
- `docs/AIS01_bin_analysis/AIS01_function_analysis.md`
- `docs/AIS01_bin_analysis/AIS01_strings.csv`
- `docs/AIS01_bin_analysis/AIS01_overview.md`
- `docs/AIS01_bin_analysis/AIS01_AT_commands.txt`
