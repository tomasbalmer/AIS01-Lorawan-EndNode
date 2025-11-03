# LoRaWAN Core (AU915)

| Function | Address | Role | References |
|----------|---------|------|------------|
| `FUN_00001214` | `0x08005214` | Bootstraps LoRaMAC context, copies AU915 defaults, wires scheduler hooks. | `docs/AIS01_bin_analysis/AIS01_function_analysis.md` (index 201), `docs/AIS01_bin_analysis/AIS01_overview.md` |
| `FUN_0000122C` | `0x0800522C` | OTAA join state handler; builds JoinRequest, tracks retries, logs `JoinRequest NbTrials`. | `AIS01_function_analysis.md` (index 203), `AIS01_strings.csv` (`0x08018887`) |
| `FUN_00001238` | `0x08005238` | Uplink serializer; assembles sensor payload, chooses FPort, notifies radio task. | `AIS01_function_analysis.md` (index 204), `AIS01_strings.csv` (`Uplink:` entries at `0x080187B0`) |
| `FUN_00001244` | `0x08005244` | Downlink demux; parses FPort/opcode table at `0x08014A00`, dispatches opcodes `0x01…0xA0`. | `AIS01_function_analysis.md` (index 205), `AIS01_overview.md` (Downlink dispatcher) |
| `FUN_00001250` | `0x08005250` | ADR controller; updates DR/TXP/NbRep and emits `ADR Message` logs. | `AIS01_function_analysis.md` (index 206), `AIS01_strings.csv` (`0x080188CA`) |
| `FUN_0000125C` | `0x0800525C` | RX window scheduler; programs RX1/RX2 timers, toggles SX1276 state machine. | `AIS01_function_analysis.md` (index 207), `AIS01_vectors.csv` (RTC/Timer IRQ) |
| `FUN_00001274` | `0x08005274` | AU915 channel calculator; maps sub-band to actual frequencies using table at `0x08014B00`. | `AIS01_function_analysis.md` (index 209), `AIS01_strings.csv` (`Subband` diagnostics) |
| `FUN_00001280` | `0x08005280` | Frame-counter manager; keeps NVM shadow of FCntUp/Down and syncs with storage routine. | `AIS01_function_analysis.md` (index 210), `AIS01_nvm_map.txt` |
| `FUN_000012D4` | `0x080052D4` | JoinAccept processor; decrypts, validates MIC, schedules MAC commands. | `AIS01_function_analysis.md` (index 217), `AIS01_strings.csv` (`Join Accept` at `0x08018828`) |
| `FUN_00001304` | `0x08005304` | DeviceTimeReq handler; aligns RTC via `AT+TIMESTAMP` infrastructure. | `AIS01_function_analysis.md` (index 221), `AIS01_strings.csv` (`timestamp error` at `0x08017670`) |

## Execution Hierarchy
```
LoRaWAN Core (0x08005214–0x08005B68)
├─ StackInit (0x08005214)
├─ JoinManager (0x0800522C)
├─ TxFlow (0x08005238)
│   └─ FrameCounterSync (0x08005280)
├─ RxFlow (0x08005244)
│   ├─ OpcodeTable@0x08014A00 (0x01..0xA0)
│   └─ DeviceTimeReq (0x08005304)
├─ ADRController (0x08005250)
└─ WindowScheduler (0x0800525C)
    └─ ChannelPlanner (0x08005274)
```

## Data Structures & Tables
- **Opcode table** at `0x08014A00`: pairs opcode → handler pointer. Downlink `0xA0` branches to calibration hook `0x08007060`; `0x01` feeds TDC update routine shared with `AT+TDC` (`0x0800603C`).
- **Channel table** at `0x08014B00`: 8-entry AU915 sub-band frequencies used by `CHE/CHS` AT handlers.
- **Event queue region** `0x08014900–0x0801497F`: scheduler nodes referenced by `FUN_00001220` (event pump) and timer ISR wrappers (`AIS01_vectors.csv`).
- **NVM shadow** addresses `0x08080808+`: frame counters and ADR state mirrored by `FUN_00001280` and persisted via storage module (`0x08007030`).

## Interaction Notes
- `AT+JOIN` (`0x08006030`) invokes `FUN_0000122C`, while `AT+ADR`, `AT+DR`, `AT+TXP` (0x08006048/54/60) eventually call into `FUN_00001250` to apply runtime changes before persisting.
- Downlink opcode `0x01` (Set TDC) reuses the same validator used by `AT+TDC` and queues an uplink confirmation through `FUN_00001238`.
- Downlink opcode `0xA0` triggers remote calibration path: `FUN_00001244` → `0x08007060` (calibration module) → schedules acknowledgement uplink via `FUN_00001238`.
- DeviceTimeReq responses update the RTC via `FUN_00005304`, synchronising with `AT+TIMESTAMP` control path and RTC configuration routine `0x08007024`.

## How This Module Fits the Firmware Flow
`main.c` hands over network lifecycle tasks to this block after boot. `FUN_00001214` seeds the LoRaMAC context, then the application state machine alternates between join (`FUN_0000122C`), transmit (`FUN_00001238`), and receive (`FUN_00001244`). Scheduler glue (`FUN_00001220`, `FUN_0000125C`) bridges hardware timers (RTC/LPTIM) with the radio driver (`0x0800700C`), ensuring RX windows open at the right time while STOP mode is active. Configuration updates arrive either from UART (`AT_*` handlers) or from downlinks; both converge here for validation, persistence (via `0x08007030`), and radio reconfiguration. The core also pushes debug strings (`AIS01_strings.csv`) used by the stock firmware, which we must reproduce for compatibility with Dragino tools.

## References
- `docs/AIS01_bin_analysis/AIS01_function_analysis.md`
- `docs/AIS01_bin_analysis/AIS01_overview.md`
- `docs/AIS01_bin_analysis/AIS01_strings.csv`
- `docs/AIS01_bin_analysis/AIS01_vectors.csv`
- `docs/AIS01_bin_analysis/AIS01_nvm_map.txt`
