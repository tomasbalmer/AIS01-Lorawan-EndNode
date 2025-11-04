# Test Plan — AIS01-LB Custom Firmware

## Setup
- Toolchain: `arm-none-eabi-gcc` (version >= 10) + `make`.
- Hardware: Dragino AIS01-LB, serial adapter (115200 8N1), calibrated gateway.
- Firmware build: run `make` to produce `build/ais01.bin` (offset 0x08004000).

## 1. Boot & Identification
1. Flash firmware via Dragino OTA Tool.
2. Power-cycle device; capture UART banner.
   - Expect strings: `Image Version: v1.0.5`, `LoRaWan Stack: DR-LWS-007`.
3. Issue `AT+VER`.
   - Expect `\r\nOK\r\n` followed by firmware info identical to stock.

## 2. AT Command Regression
| Command | Input | Expected UART Response |
|---------|-------|------------------------|
| `AT+DEVEUI?` | — | `+DEVEUI:<hex>` then `\r\nOK\r\n` |
| `AT+JOIN=1` | — | `\r\nOK\r\n` then `JOINED` or appropriate error |
| `AT+ADR=1` | — | `\r\nOK\r\n` |
| `AT+CALIBREMOTE=<payload>` | apply command | `+CALIBREMOTE:<echo>` then `\r\nOK\r\n` |
| `AT+CALIBREMOTE=00` | query | `+CALIBREMOTE:<mirror>` then `\r\nOK\r\n` |

Record behaviour for invalid payload (odd length → `AT_PARAM_ERROR`).

## 3. Downlink Regression
1. Send downlink opcode `0x01` (Set TDC) with payload `0x01 0x10 0x27 0x00 0x00` → expect new TDC printed by `AT+TDC?`.
2. Send opcode `0x21` (ADR=1) → `AT+ADR?` returns 1.
3. Send opcode `0xA0` (calibration apply) with payload matching the AT case → gateway should show uplink ack `OK` and UART prints identical `+CALIBREMOTE` line.
4. Verify that repeated apply updates mirror (`AT+CALIBREMOTE=00` returns latest data).

## 4. Power & Scheduler
- Trigger `AT+CALIBREMOTE=<apply>` and, while busy, monitor that the device does **not** enter STOP mode (no long gaps, debug prints `Calibration active, skipping STOP mode`).
- Once apply completes, observe timer-driven sleep (`Power_EnterStopMode` debug `Entering STOP mode ...`).
- Measure current during STOP (<20 µA target) using multimeter or current probe.

## 5. Persistence (optional)
- Power-cycle after calibration. Expect calibration to reset (since firmware mirrors original behaviour). Document if persistence enhancement is later introduced.

## 6. Gateway Integration
- Join network (OTAA or ABP). Confirm uplinks appear in Dragino/AWS console.
- Evaluate RSSI/SNR updates (`AT+RSSI?`, `AT+SNR?`) post downlink.

## 7. Regression Checklist
- No crashes or stuck states after repeated CALIBREMOTE apply/query cycles.
- `AT+LOWPOWER` or equivalent low-power commands continue to function.
- `AT+CFG` prints consistent configuration including last set TDC/ADR.

## Reporting
Log UART transcripts and gateway events for each section. Flag discrepancies vs stock firmware outputs for follow-up.
