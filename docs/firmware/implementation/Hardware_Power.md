# Hardware & Power Services

| Function | Address | Responsibility | Notes & References |
|----------|---------|----------------|--------------------|
| `FUN_00003000` | `0x08007000` | Board bring-up; clocks, GPIO default states, SX1276 pin map (PA1/PA2/PA3/PB12…) and UART mux. | `docs/AIS01_bin_analysis/AIS01_function_analysis.md` (501), `AIS01_vectors.csv` (Reset handler) |
| `FUN_0000300C` | `0x0800700C` | SX1276 radio driver glue; toggles NSS/RESET, handles DIO IRQ bridging to LoRa core. | `AIS01_function_analysis.md` (502), `AIS01_overview.md` (radio strings) |
| `FUN_00003018` | `0x08007018` | STOP-mode manager; shuts down clocks, configures `PWR->CR1`, arms RTC wake-up. | `AIS01_function_analysis.md` (503), `AIS01_strings.csv` (`SLEEP` at `0x08017380`) |
| `FUN_00003024` | `0x08007024` | RTC/LSE configuration and timestamp sync; shared with `AT+TIMESTAMP`. | `AIS01_function_analysis.md` (504), `AIS01_strings.csv` (`Set current timestamp=%u`) |
| `FUN_00003030` | `0x08007030` | Flash/EEPROM persistence layer; verifies magic, CRC, services `AT+CFG`, frame counters. | `AIS01_function_analysis.md` (505), `AIS01_nvm_map.txt` |
| `FUN_0000303C` | `0x0800703C` | Sensor UART bridge; detects AI module (“AIS01_LB Detected”), manages JPEG fetch. | `AIS01_function_analysis.md` (506), `AIS01_strings.csv` (`AIS01_LB Detected` `0x08017366`) |
| `FUN_00003048` | `0x08007048` | JPEG buffers; tracks `jpeg_flag`/`jpeg_size`, stores frames to flash for OTA retrieval. | `AIS01_function_analysis.md` (507), `AIS01_strings.csv` (`jpeg_size:%d` `0x08017409`) |
| `FUN_00003054` | `0x08007054` | Battery monitor; samples ADC, scales to mV for `AT+BAT`. | `AIS01_function_analysis.md` (508), `AIS01_strings.csv` (`Bat_voltage:%d mv`) |
| `FUN_00003060` | `0x08007060` | Remote calibration executor; decodes payload (AT or opcode `0xA0`), updates calibration slots, sets deferred-apply flag. | `AIS01_function_analysis.md` (509), `AIS01_overview.md` (remote calibration) |
| `FUN_0000306C` | `0x0800706C` | Factory-default reloader; wipes EEPROM region `0x08080800+`, restores shipping config. | `AIS01_function_analysis.md` (510), `AIS01_nvm_map.txt` |

## Peripheral Tree
```
Hardware Layer (0x08007000–0x0800742C)
├─ Init & Clocks (0x08007000, 0x080070C0)
│   ├─ GPIO Init (0x08007078)
│   └─ SX1276 Wiring (0x0800700C)
├─ Power Control
│   ├─ StopMode (0x08007018)
│   ├─ RTC (0x08007024)
│   └─ Battery ADC (0x08007054)
├─ Storage (0x08007030)
│   └─ Factory Reset (0x0800706C)
├─ Sensor Stack
│   ├─ AIS01 UART Bridge (0x0800703C)
│   ├─ JPEG Manager (0x08007048)
│   └─ Calibration Engine (0x08007060)
└─ HAL Wrappers (0x080070E4–0x0800742C)
    └─ STM32L0 peripheral stubs (UART/SPI/I2C/DMA, etc.) retained from original firmware
```

## Cross-Module Touchpoints
- **LoRaWAN Core:** `FUN_0000300C` exposes SPI/DIO toggles used by the LoRa state machine (`0x0800525C`). RX window timestamps rely on the RTC services (`0x08007024`).
- **AT Layer:** Commands `AT+BAT`, `AT+SLEEP`, `AT+5VT`, `AT+INTMODx`, `AT+GETSENSORVALUE`, `AT+JPEGSIZE`, `AT+CALIBREMOTE` all terminate here. Parameter updates are acknowledged by returning standard strings (see `AIS01_strings.csv`).
- **Storage:** `FUN_00003030` and `FUN_0000306C` are invoked by both AT commands and downlink actions needing persistence, including frame counter sync from `0x08005280`.
- **Remote Calibration Flow:** AT or downlink payload → `FUN_00003060` writes provisional calibration values, sets “pending apply” flag stored alongside sensor coefficients, and optionally arms a confirmation uplink through `FUN_00001238`.

## How Hardware & Power Fit the Firmware Flow
During BOOT, `FUN_00003000` configures system clocks (MSI → PLL), powers the SX1276, and reads persisted configuration. Before entering the main loop, the firmware primes the sensor UART and JPEG buffers so the first capture is ready for uplink. Throughout runtime, the application alternates between active radio windows and STOP mode; `FUN_00003018` ensures the MCU drops below 20 µA by disabling high-speed clocks and gating GPIOs, while the RTC service (`0x08007024`) re-establishes the crystal clock after wake-up. When configuration changes require persistence, the storage layer commits to EEPROM, preserving battery life by batching writes. All hardware-centric strings (battery, sleep, sensor detection) live in `AIS01_strings.csv`, guiding us to replicate the same user-facing outputs in the custom firmware.
