AIS01-Lorawan-EndNode Firmware

High-quality, production-architecture firmware for the Dragino AIS01-LB LoRaWAN AI Image Sensor, designed for:
	•	LoRaWAN AU915 Sub-Band 2 (OEM-aligned)
	•	Class A OTAA operation
	•	Deep low-power operation suitable for long-term battery deployments
	•	OEM-parity payloads, AT commands, and calibration workflows
	•	Laboratory-grade stability for internal development and testing

This repository represents a full-stack rewrite of the AIS01-LB firmware, created through:
	•	hardware bring-up,
	•	reverse engineering of the OEM firmware,
	•	LoRaWAN stack reconstruction,
	•	careful architectural design,
	•	and a fully open, maintainable codebase.

The firmware aims for full functional parity with the Dragino OEM firmware while providing an entirely transparent, auditable implementation suitable for R&D.

⸻

📘 Table of Contents
	1.	Project Goals￼
	2.	Key Features￼
	3.	Architecture￼
	4.	Repository Layout￼
	5.	Build System￼
	6.	Flashing Instructions￼
	7.	AT Command Interface￼
	8.	LoRaWAN Payload Formats (OEM-Aligned)￼
	9.	Calibration Workflow￼
	10.	Low-Power Design￼
	11.	Roadmap￼
	12.	Documentation￼
	13.	Development Notes￼
	14.	License￼

⸻

1. Project Goals

The primary objectives of this firmware are:

🎯 OEM Parity

Match Dragino AIS01-LB OEM behavior:
	•	Uplink formats
	•	AT commands
	•	Calibration flows
	•	Bootloader compatibility

🎯 Transparency & Maintainability

Unlike the OEM binary, this firmware is:
	•	fully readable,
	•	well-architected,
	•	thoroughly documented,
	•	designed for long-term evolution.

🎯 Laboratory Stability

Version 1 focuses on internal reliability while providing hooks for future production deployments.

🎯 Extendability for R&D

The architecture is modular enough to:
	•	add features without regressions,
	•	onboard new peripherals,
	•	expand LoRaWAN behavior,
	•	implement advanced calibration.

⸻

2. Key Features

✔ LoRaWAN AU915 (Sub-Band 2)
	•	Class A OTAA
	•	OEM-style join and uplink behavior
	•	Configurable uplink interval (TDC)

✔ AT Command System
	•	Full set of AT commands for provisioning
	•	Boot-time safe AT window
	•	Runtime AT stability (never locked out)

✔ OEM-Aligned Uplinks
	•	FPORT=2 periodic sensor reading
	•	FPORT=5 device status (band, battery, model, fw)
	•	FPORT=3 image transfer (basic v1 trigger)

✔ Calibration Engine
	•	Local calibration (UART + PC Tool)
	•	Remote calibration (minimal v1)
	•	ROI/digit wheel definitions aligned with Dragino documentation

✔ Low-Power Operation
	•	STOP mode + RTC
	•	Peripheral clock gating
	•	Radio sleep scheduling
	•	Watchdog-friendly sleep segmentation

✔ Reliability & Safety
	•	Integrated IWDG watchdog
	•	Clean error recovery paths
	•	NV storage with CRC32 state protection

⸻

3. Architecture

The firmware follows a layered, decoupled architecture common in high-reliability embedded systems.

+-----------------------------------------------------------+
| Application Layer (state machine, AT, calibration, sensor) |
+---------------------------+-------------------------------+
| LoRaWAN MAC Layer         | Calibration Engine           |
+---------------------------+-------------------------------+
| Board Support Package (STM32L072 + SX1276)               |
+---------------------------+-------------------------------+
| System Utilities (timers, uart, spi, fifo, crc32, power) |
+-----------------------------------------------------------+
| Low-level CMSIS / Startup / Linker / Bootloader interface |
+-----------------------------------------------------------+

Design Principles:
	•	Small, pure C modules
	•	Explicit interfaces
	•	No hidden globals
	•	Predictable state transitions
	•	Clean separation between “logic” and “hardware”

⸻

4. Repository Layout

AIS01-Lorawan-EndNode/
├── src/
│   ├── app/        # Application logic: AT, state machine, calibration engine
│   ├── board/      # MCU board support package (GPIO, UART, SPI, RTC, IWDG, SX1276)
│   ├── cmsis/      # ARM CMSIS + STM32 startup and clocks
│   ├── lorawan/    # LoRaWAN MAC, crypto, region tables, AU915 implementation
│   ├── radio/      # SX1276 radio driver
│   └── system/     # Drivers/utilities: timers, crc32, scheduler, fifo, uart, spi, adc
├── docs/
│   ├── firmware/   # Engineering specs, payload mapping, AT dictionary
│   ├── analysis/   # Reverse-engineering notes of OEM firmware
│   ├── reports/    # System scans, engineering discussions
│   └── roadmap/    # Roadmap for v1-lab and future versions
├── Makefile        # GNU Make build
└── stm32l072xx_flash_app.ld  # Linker script (app at 0x08004000)


⸻

5. Build System

Requirements
	•	ARM GCC (arm-none-eabi-gcc) 10.x–14.x
	•	GNU Make
	•	Optional: Python 3 for automation tooling

Build

make clean
make -j4

Artifacts produced in build/:
	•	ais01.bin  → Flashable file (OTA/SWD)
	•	ais01.elf  → Debuggable image
	•	ais01.hex  → Alternative flashing format
	•	.map, .lst files

⸻

6. Flashing Instructions

✔ Dragino OTA Bootloader (Recommended)
	1.	Run the Dragino OTA Tool
	2.	Select ais01.bin
	3.	Flash to address 0x08004000

✔ ST-Link (SWD)

st-flash write build/ais01.bin 0x08004000

✔ Bootloader Compatibility

The binary layout respects Dragino’s memory map:
	•	Bootloader occupies: 0x08000000–0x08003FFF
	•	Application starts at: 0x08004000

⸻

7. AT Command Interface

Detailed specification: docs/firmware/specification/AT_Commands.md

Core Commands

Command	Meaning
AT	Ping / alive check
ATZ	Soft reboot
AT+VER	Firmware version
AT+FDR	Factory defaults

LoRaWAN Provisioning

Command	Description
AT+DEVEUI=<hex>	Set DevEUI
AT+APPEUI=<hex>	Set AppEUI
AT+APPKEY=<hex>	Set AppKey
AT+JOIN	Initiate OTAA join

Operation & Power

Command	Description
AT+TDC=<ms>	Uplink interval
AT+DR=<n>	DataRate
AT+ADR=<0/1>	ADR control
AT+TIMESTAMP=<epoch>	Set timestamp

Calibration

Command	Description
AT+CALIBREMOTE=<payload>	Apply remote calibration block


⸻

8. LoRaWAN Payload Formats (OEM-Aligned)

See: docs/firmware/specification/Payload_Mapping.md

FPORT = 2 — Periodic Reading

[0..1]   Battery (mV)
[2..5]   Unix Timestamp
[6..9]   Integer reading
[10..13] Decimal reading (scaled)
[14]     Detection Mark (0x01 = camera OK)

FPORT = 5 — Device Status

Model (0x1C)
Firmware Version
Frequency Band (AU915 → 0x04)
Subband
Battery (mV)

FPORT = 3 — Image Data (Minimal v1)

Triggered via downlink 0x0B 01.
Transmits JPEG chunks using OEM header format.

⸻

9. Calibration Workflow

Local Calibration (Recommended)
	•	PC Tool via UART
	•	ROI coordinate definition
	•	Digital wheel and decimal alignment

Remote Calibration (Minimal v1-Lab)
	•	Single snapshot calibration payload
	•	Applied atomically
	•	Persistence configurable

Documentation: docs/firmware/implementation/Calibration_Engine.md

⸻

10. Low-Power综合在线
	•	STOP mode with RTC wake
	•	SX1276 powered down when idle
	•	UART/SPI/I2C/ADC clock gating
	•	Watchdog-friendly segmented sleeps

Engineering notes: docs/firmware/implementation/Hardware_Power.md

⸻

11. Roadmap

Current roadmap is maintained in:

docs/roadmap/v1_lab_roadmap.md

Version 1 (Lab) focuses on:
	•	AT stability + boot window
	•	OEM uplink parity (FPORT=2,5)
	•	Power gating finalization
	•	Calibration flow reliability

⸻

12. Documentation

All documentation lives under docs/:

Firmware Specifications

docs/firmware/

Reverse Engineering Notes

docs/analysis/, docs/reports/

Roadmaps

docs/roadmap/

⸻

13. Development Notes

For engineering decisions, reversals, rationale, and deep analysis, see:

docs/reports/

These capture the evolution of the firmware and insights from OEM reverse engineering.

⸻

14. License

Internal testing firmware for R&D and evaluation purposes.

⸻

15. Contact

Engineering notes, discussions, and historical design records are under:

docs/reports/

Feel free to explore for detailed technical reasoning behind each subsystem.