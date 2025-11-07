# Firmware Rebuild Implementation Plan

## Scope & Baseline
- Target: Dragino AIS01-LB (STM32L072CZ + SX1276, AU915 Sub-band 2) running a clean-room firmware that mirrors OEM behaviour (AT command surface, downlink opcodes, sensor pipeline, STOP-mode power profile) while enabling future customization.
- Toolchain: GNU Make + `arm-none-eabi-gcc`, linker offset `0x08004000`, leveraging in-tree LoRaWAN stack (`src/lorawan`) and board support glue under `src/board`.
- Deliverables: production-quality C sources/headers, build artifacts (.elf/.bin), validation logs, and reverse-engineering notes archived in `docs/` + `reports/`.

## Current Implementation Snapshot
- **LoRaWAN core (`src/lorawan`)**: Custom Class-A implementation with join/tx/rx scheduling, ADR hooks, AU915 region helpers, and callbacks to `lorawan_app.c`. Needs verification for MAC command coverage, sub-band masks, confirmed uplink retries, DeviceTimeReq handling, and parity with OEM downlink dispatcher strings.
- **Application state machine (`src/app/main.c`)**: Boot→Join→Idle→Tx/Sleep loop, periodic timer, uplink payload builder (battery byte + two 16-bit sensor readings), and UART-fed AT parser integration.
- **AT command layer (`src/app/atcmd.c`)**: Large handler table covering credentials, radio params, timers, debug/power hooks, and calibration entry points. Parser currently ignores OEM password gate, response throttling, and some help-text mappings; needs audit against `docs/rebuild/AT_Handlers.md` / `AIS01_strings.md`.
- **Storage (`src/app/storage.c` + `eeprom-board.c`)**: CRC-protected struct persisted to internal EEPROM. Defaults roughly match `config.h`, but layout diverges from OEM `AIS01_nvm_map.md` (missing timestamp, SyncMod, JPEG flags, calibration mirrors). Factory reset path stubbed via `Storage_Init` defaults only.
- **Calibration engine (`src/app/calibration.c` + `src/board/calibration_hw.c`)**: Implements buffer/mirror logic and hardware toggles derived from RE notes; persistence deferred to callers. Needs tie-in to AT & downlink acknowledgements.
- **Sensor subsystem (`src/app/sensor.c`)**: Core scheduler plus calibration math now backed by a real AIS01 bridge (`src/board/sensor-board.c`) that drives a dedicated UART (USART1), handles power enable, handshake, command framing, and frame buffering for `AT+GETSENSORVALUE`/JPEG paths.
- **Power & battery instrumentation**: STOP-mode framework exists, and board layer now delivers ADC-based battery measurements (`BoardBatteryMeasureVoltage`/`AT+BAT`) using the actual divider enable pin. Peripheral gating and wake-source attribution still TODO; RTC + GPIO clocks assumed but not confirmed.
- **Board support / drivers (`src/board`)**: GPIO/UART/SPI/LPM implemented; ADC, RTC, SX1276 shims mostly skeletal. `BoardInitPeriph` is empty, `BoardBattery*` return 0, EEPROM + calibration hooks exist but sensor rail, 5V boost, and timestamp plumbing are missing.
- **Tooling/tests**: Makefile builds but no CI, unit tests, or hardware-in-loop scripts. `docs/rebuild/Test_Plan.md` defines manual procedures only.

## Known Unknowns / Pending Data
1. **Downlink opcode table beyond documented opcodes**: `docs/rebuild/Downlink_Dispatcher.md` lists only partial mapping. Need the remaining entries (e.g., power control, JPEG retrieval, timestamp sync) plus associated payload layouts to ensure LoRa downlinks stay compatible with Dragino tooling.
2. **Sensor frame semantics**: Although the bridge now mirrors the OEM command width (opcode + 24-bit parameter), we still need confirmation of the exact payload layout for JPEG control and wake-interval negotiation to guarantee interoperability.
3. **STOP-mode gating**: With the sensor bridge active we must confirm which peripherals remain clocked during STOP and how the OEM scheduler deferred sensor work while the MCU slept.

## Next Objectives & Validation
1. **LoRa/AT parity hardening**  
   - Cross-check AT handlers, downlink dispatcher, and storage layout against RE docs. Align responses/strings, ensure mirrored validators for AT + downlink paths, and persist new fields (timestamp, JPEG flags, calibration state).  
   - Validation: scripted AT regression (per `docs/rebuild/Test_Plan.md`) and gateway captures showing opcodes `0x01/0x21/0xA0` behave identically to OEM.
2. **Low-power & scheduling polish**  
   - Flesh out `Power_Disable/EnablePeripherals`, confirm RTC/LSE/timer configuration, and integrate sensor/calibration busy flags to respect STOP-mode entry constraints documented under `docs/rebuild/Scheduler.md`.  
   - Validation: current draw <20 µA in STOP, wake-on-RTC trace aligned with `TxDutyCycle`, and debug logs mirroring OEM sleep messages.
3. **Sensor/JPEG protocol validation**  
   - Capture live traffic from the AIS module using the new bridge to confirm opcode semantics, JPEG transfer flow, and wake-interval updates; update documentation/tests accordingly.  
   - Validation: UART logs demonstrating `AT+GETSENSORVALUE`, `AT+JPEGSIZE`, and JPEG retrieval responses match the OEM strings and timing.

## Planned Agent A Data Requests
- Dump/decompile OEM routines at `0x0800703C–0x08007048` (sensor/JPEG path) including GPIO writes, UART parameters, and any string references to correlate with AT commands.
- Dump/decompile OEM battery/ADC routine at `0x08007054–0x0800706B`, capturing math constants and LUTs that map ADC readings to millivolts/percent.
- Extract the full downlink opcode table located near `0x08014ABC–0x08014B40`, annotating opcode values, handler targets, and linked strings.

Each request will be tracked in `/reports/<task>-report.md` once Agent A returns findings, and this plan will be updated as modules move from “unknown” to “implemented/validated”.
