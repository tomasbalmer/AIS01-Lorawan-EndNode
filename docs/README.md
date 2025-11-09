# AIS01-LB LoRaWAN End Node — Documentation

**Device:** Dragino AIS01-LB (LoRaWAN AI Image End Node)
**MCU:** STM32L072CZ
**Radio:** SX1276 (LoRa)
**Firmware Version:** v1.0.5
**LoRaWAN Stack:** DR-LWS-007 (AU915)

---

## Documentation Structure

This documentation is organized into three main categories:

```
docs/
├── analysis/           # Reverse engineering artifacts from original firmware
├── specification/      # Consolidated technical specifications
├── implementation/     # Implementation guides for rebuilding firmware
├── reports/           # Progress reports and scans
└── prompts/           # Agent prompts for automated analysis
```

---

## Quick Start Guide

### For Understanding the Original Firmware
Start with:
1. `specification/Architecture_Specification.md` — System overview and module map
2. `analysis/AIS01_overview.md` — Binary analysis summary
3. `specification/AT_Commands_Specification.md` — Complete AT command reference

### For Implementing New Firmware
Start with:
1. `specification/Architecture_Specification.md` — System architecture
2. `implementation/` folder — Module-by-module implementation guides
3. `specification/LoRaWAN_Core_Specification.md` — LoRaWAN stack details

### For Hardware Integration
Start with:
1. `specification/Hardware_Specification.md` — Component datasheets and pinout
2. `analysis/AIS01_nvm_map.md` — Non-volatile memory layout
3. `specification/Architecture_Specification.md` — Memory map and power management

---

## 📁 analysis/ — Reverse Engineering

**Purpose:** Raw artifacts and analysis from the original Dragino firmware binary

| File | Description |
|------|-------------|
| `AIS01_overview.md` | High-level summary of binary analysis (memory map, modules, AT commands) |
| `AIS01_function_analysis.md` | Detailed analysis of 400+ functions extracted from binary |
| `AIS01_extraction_plan.md` | Reverse engineering roadmap and pending analysis tasks |
| `AIS01_nvm_map.md` | Non-volatile memory layout (EEPROM shadow regions) |
| `AIS01_strings.md` | String literals and messages extracted from binary |
| `AIS01_pointers.csv` | Pointer tables (AT tables, state-machine jumps, radio callbacks) |
| `AIS01_vectors.csv` | Interrupt vector table with handlers and addresses |
| `AIS01_AT_commands_legacy.md` | Legacy AT command reference (see specification/ for consolidated version) |
| `AT_Response_Map_legacy.md` | Legacy response map (consolidated into specification/) |
| `Firmware_Architecture_Map_legacy.md` | Legacy architecture map (consolidated into specification/) |

**Key Use Cases:**
- Understanding how the original firmware works at the binary level
- Finding specific addresses and function implementations
- Validating assumptions about firmware behavior
- Reference for string messages to maintain tool compatibility

---

## 📁 specification/ — Technical Specifications

**Purpose:** Consolidated, authoritative technical specifications for the AIS01-LB firmware

| File | Description |
|------|-------------|
| `Architecture_Specification.md` | **Master architecture document** — System design, memory map, module topology, execution flow |
| `AT_Commands_Specification.md` | **Complete AT command reference** — All 67 commands with syntax, parameters, handlers, and responses |
| `LoRaWAN_Core_Specification.md` | LoRaWAN stack implementation (AU915 region, join, uplink, downlink, ADR) |
| `Hardware_Specification.md` | Hardware documentation guide — Datasheets, pinout, component specs, what to download |

**Key Use Cases:**
- Primary reference for firmware development
- Understanding system architecture and design decisions
- AT command integration and validation
- LoRaWAN stack behavior and compliance
- Hardware integration planning

**Start Here:** If you're new to the project, read `Architecture_Specification.md` first.

---

## 📁 implementation/ — Implementation Guides

**Purpose:** Module-by-module guides for rebuilding/reimplementing the firmware

| File | Description |
|------|-------------|
| `AT_Handlers.md` | AT command parser implementation (table structure, handlers, validation) |
| `Calibration_Engine.md` | Remote calibration system (downlink opcodes, payload handling, hardware interface) |
| `Downlink_Dispatcher.md` | Downlink opcode dispatcher and handler routing |
| `Hardware_Power.md` | Power management and hardware abstraction layer |
| `Scheduler.md` | Event scheduler and timing management |

**Key Use Cases:**
- Step-by-step guides for implementing each firmware module
- Understanding handler interactions and data flow
- Replicating original firmware behavior
- Integration patterns between modules

---

## 📁 reports/ — Progress Reports

**Purpose:** Analysis reports and system scans

| File | Description |
|------|-------------|
| `20251107_FULL_SYSTEM_SCAN.md` | Full system scan from November 7, 2025 |
| `ARCHITECTURE.md` | Architecture analysis report (consolidated into specification/) |
| `HARDWARE_DOCUMENTATION_NEEDED.md` | Hardware documentation status (moved to specification/) |
| `REMAINING_FEATURES_ANALYSIS.md` | Remaining features to implement/analyze |

---

## 📁 prompts/ — Agent Prompts

**Purpose:** Automated analysis prompts for AI agents

| File | Description |
|------|-------------|
| `agent-a.md` | Agent A prompt configuration |
| `agent-b.md` | Agent B prompt configuration |

---

## Document Relationships

### Cross-Reference Map

```
Architecture_Specification.md (MASTER)
    ├─→ AT_Commands_Specification.md
    │   └─→ analysis/AIS01_strings.md (response strings)
    ├─→ LoRaWAN_Core_Specification.md
    │   └─→ analysis/AIS01_function_analysis.md (binary functions)
    ├─→ Hardware_Specification.md
    │   └─→ analysis/AIS01_nvm_map.md (memory layout)
    └─→ implementation/ modules
        ├─→ AT_Handlers.md
        ├─→ Calibration_Engine.md
        ├─→ Downlink_Dispatcher.md
        ├─→ Hardware_Power.md
        └─→ Scheduler.md

analysis/AIS01_overview.md
    ├─→ All analysis/*.csv files
    └─→ analysis/AIS01_extraction_plan.md
```

---

## Recommended Reading Order

### For Developers (Building New Firmware)

1. **Day 1 — Architecture Understanding**
   - `specification/Architecture_Specification.md` (2 hours)
   - `specification/Hardware_Specification.md` (30 min)
   - Download datasheets listed in Hardware_Specification.md

2. **Day 2 — LoRaWAN Stack**
   - `specification/LoRaWAN_Core_Specification.md` (1 hour)
   - `implementation/Scheduler.md` (30 min)
   - `implementation/Hardware_Power.md` (30 min)

3. **Day 3 — AT Commands**
   - `specification/AT_Commands_Specification.md` (1 hour)
   - `implementation/AT_Handlers.md` (1 hour)

4. **Day 4 — Downlinks & Calibration**
   - `implementation/Downlink_Dispatcher.md` (30 min)
   - `implementation/Calibration_Engine.md` (30 min)

5. **Day 5+ — Implementation**
   - Use implementation/ guides as module-by-module references
   - Cross-reference with analysis/ for binary validation

### For Reverse Engineers (Understanding Original Firmware)

1. **Start Here:**
   - `analysis/AIS01_overview.md` (30 min overview)
   - `analysis/AIS01_extraction_plan.md` (roadmap)

2. **Deep Dive:**
   - `analysis/AIS01_function_analysis.md` (function catalog)
   - `analysis/AIS01_strings.md` (message strings)
   - `analysis/AIS01_pointers.csv` + `AIS01_vectors.csv` (tables)

3. **Cross-Check with Specs:**
   - Compare findings with `specification/` documents
   - Validate against `implementation/` guides

---

## Key Concepts

### Memory Addresses

**Important:** Binary dump analyzed covers `0x08000000–0x0801502F`. References to addresses beyond `0x0801xxxx` are external dependencies (bootloader or precompiled libraries).

**Flash Layout:**
- `0x08000000–0x08003FFF`: Bootloader (16 KB, Dragino proprietary)
- `0x08004000–0x0802FFFF`: Application (176 KB)
- `0x08080000–0x08080FFF`: Data EEPROM (4 KB)

**RAM Layout:**
- `0x20000000–0x20003FFF`: Variables + Stack (16 KB)
- `0x20004000–0x20004FFF`: Heap (4 KB)

### AT Command Parser

**Table Location:** `0x08016A06–0x08016A68` (67 entries)
**Parser Entry:** `FUN_00002000` @ `0x08006000`

**Structure:**
```c
typedef struct {
    const char *name;           // Command name (without "AT+")
    ATCmdHandler_t handler;     // Function pointer
    const char *help;           // Help text pointer
    uint8_t flags;              // Access flags
} ATCmdEntry_t;
```

### LoRaWAN Stack

**Key Functions:**
- StackInit: `0x08005214`
- Join: `0x0800522C`
- Uplink: `0x08005238`
- Downlink: `0x08005244`
- ADR Controller: `0x08005250`
- RX Scheduler: `0x0800525C`

### Power States

```c
POWER_MODE_RUN    // ~1.5 mA (CPU active)
POWER_MODE_SLEEP  // ~500 µA (WFI)
POWER_MODE_STOP   // <20 µA (RTC + flash standby)
```

---

## External Resources

### Official Documentation
- **Dragino Wiki:** http://wiki.dragino.com/xwiki/bin/view/Main/User%20Manual%20for%20LoRaWAN%20End%20Nodes/AIS01-LB--LoRaWAN_AI_Image_End_Node_User_Manual/
- **GitHub Decoders:** https://github.com/dragino/dragino-end-node-decoder/tree/main/AIS01

### Datasheets (Download Links in Hardware_Specification.md)
- **STM32L072CZ:** https://www.st.com/en/microcontrollers-microprocessors/stm32l072cz.html
- **SX1276:** https://www.semtech.com/products/wireless-rf/lora-core/sx1276
- **LoRaWAN Spec:** https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-3/

### Tools
- **STM32CubeMX:** https://www.st.com/en/development-tools/stm32cubemx.html (clock/pin config)
- **Ghidra:** https://ghidra-sre.org/ (reverse engineering)

---

## Documentation Maintenance

### Updating Documentation

1. **Binary Analysis Changes:**
   - Update files in `analysis/`
   - Update `analysis/AIS01_extraction_plan.md` with progress
   - Cross-reference with `specification/` to ensure consistency

2. **Specification Changes:**
   - Update master specs in `specification/`
   - Ensure implementation/ guides reflect changes
   - Update cross-references in this README

3. **Implementation Changes:**
   - Update guides in `implementation/`
   - Validate against binary in `analysis/`
   - Update Architecture_Specification.md if needed

### Document Status

| Category | Status | Last Updated |
|----------|--------|--------------|
| analysis/ | ✅ Complete | 2025-11-03 |
| specification/ | ✅ Consolidated | 2025-11-08 |
| implementation/ | ✅ Complete | 2025-11-03 |
| reports/ | 🔄 Ongoing | 2025-11-07 |

---

## Contributing

When adding new documentation:

1. **Determine Category:**
   - Binary analysis → `analysis/`
   - Technical specification → `specification/`
   - Implementation guide → `implementation/`
   - Progress report → `reports/`

2. **Update Cross-References:**
   - Add links from related documents
   - Update this README index
   - Update Architecture_Specification.md if needed

3. **Follow Naming Conventions:**
   - `analysis/`: Prefix with `AIS01_`
   - `specification/`: Suffix with `_Specification.md`
   - `implementation/`: Module name (e.g., `AT_Handlers.md`)

---

## FAQ

### Q: Where do I start if I want to build custom firmware?
**A:** Read `specification/Architecture_Specification.md` first, then follow the "For Developers" reading order above.

### Q: How do I find a specific AT command implementation?
**A:** Check `specification/AT_Commands_Specification.md` for the command details and handler address, then reference `implementation/AT_Handlers.md` for implementation guidance.

### Q: What's the difference between analysis/ and specification/?
**A:**
- `analysis/` = Raw reverse engineering data from the binary
- `specification/` = Cleaned, consolidated technical specs for reference

### Q: Can I skip the binary analysis files?
**A:** Yes, if you're only implementing new firmware. Use `specification/` and `implementation/` as your primary references. Refer to `analysis/` only when you need to validate against the original binary.

### Q: Where are the datasheets?
**A:** Download links are in `specification/Hardware_Specification.md`. Most are free from manufacturer websites.

### Q: How do I maintain compatibility with Dragino tools?
**A:** Keep response strings identical to the original firmware. See `analysis/AIS01_strings.md` for the complete list and `specification/AT_Commands_Specification.md` for response specifications.

---

## Document Change Log

| Date | Change | Files Affected |
|------|--------|----------------|
| 2025-11-08 | Consolidated documentation into 3-folder structure | All |
| 2025-11-08 | Created master specification documents | specification/*.md |
| 2025-11-08 | Created this README index | README.md |
| 2025-11-03 | Completed binary analysis | analysis/*.md, analysis/*.csv |
| 2025-11-03 | Created implementation guides | implementation/*.md |

---

**Last Updated:** 2025-11-08
**Documentation Version:** 2.0 (Consolidated)
