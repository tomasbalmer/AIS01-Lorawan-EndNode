# AIS01-LB Firmware (AU915) — Overview from `ghydra.txt`

## Basic Coordinates
- **Application Offset:** The Ghidra dump starts at `0x0000`, but the real image loads at `0x0800_4000`. Add this offset to obtain absolute addresses (e.g., `Reset` in listing → `0x0800F30D`).
- **Image Size:** 0x15030 bytes ≈ 86 kB, consistent with the original `.bin`.
- **Vector Table:** `0x0800_4000–0x0800_40FF`. Reset → `0x0800F30D`; most IRQs point to routines in `0x080147xx–0x080148xx`, all sharing common handlers.
- **Code Sections:** High density between `0x08004100–0x08016A00` (585 recognized functions). Clear grouping by function (HAL, LoRa, AT).
- **BSS/ZI:** Section `0x08014900–0x0801502F` filled with `0x00`, ending with word `0x20006D58`; indicates buffers/image caches and state structures.

---

## Module Overview (Heuristic by Range)
- `0x08004100–0x08005FFF`: Math/HAL primitives (many reused `FUN_00000xxx`). Includes common routines (`FUN_00000148`, `FUN_0000014C`).
- `0x08006000–0x08008FFF`: Peripheral controllers (SPI/UART/RTC). Pointers around `0x08008770` suggest a callback table.
- `0x08009000–0x0800DFFF`: LoRa event queue and radio code (IRQ and timers).
- `0x0800F000–0x08011FFF`: LoRaMAC layer and join/uplink management (`FUN_0000f28c`, `FUN_0001043c`, etc.).
- `0x08012000–0x08013FFF`: AT parser logic; numerous calls to validators (`FUN_000122xx`) and configuration writers.
- `0x08014000–0x080148FF`: Monolithic block with text tables, AT state, and post-join routines.
- `0x08014900–0x08015FFF`: Working buffers (JPEG image, temporary packet storage) and pointer tables.
- `0x08016A00–0x08016BFF`: Command/help table (pointers `0x08016A0x → 0x080080xx/0x08013xxx`).

---

## AT Command Table (67 Entries)
- Command names (`+DEUI`, `+APPKEY`, `+TDC`, `+SYNCMOD`, etc.) serialized starting at `0x0801456F` (ASCII sequence `2B xx`). The full list is in `BinAnalysis/AU915_commands.txt`.
- Pointer table (`analysis/ghydra_pointers.csv`, rows `0x08016A06–0x08016A68`) links to help texts located at `0x08013D20–0x080183D8`.

**Key Categories:**
- *LoRaWAN Core:* `DEUI`, `APPEUI`, `APPKEY`, `NWKSKEY`, `APPSKEY`, `DADDR`, `ADR`, `DR`, `TXP`, `RX1DL`, `RX2DR`, `RX2FQ`, `JN1DL`, `JN2DL`, `NJM`, `CLASS`, `PNACKMD`.
- *Regional:* `CHE`, `CHS`, `RX1WTO/RX2WTO`, `DWELLT`, `RJTDC`, `SETMAXNBTRANS`.
- *Time & Sync:* `TIMESTAMP`, `SYNCMOD`, `SYNCTDC`, `LEAPSEC`, `CLOCKLOG`, `RPL`, `RJTDC`.
- *System Management:* `CFG`, `VER`, `DEBUG`, `FDR`, `PWORD`, `SLEEP`, `5VT`, `INTMOD1/2/3`.
- *AI Application:* `GETSENSORVALUE`, `PDTA`, `PLDTA`, `CLRDTA`, `UUID`, `PWRM2`.

**Validation Rules (inferred from text):**
- Minimum TDC: 4 seconds (`0x080173F9`).
- Sub-band restriction warning (`0x08017D6E`).
- Post-ADR advisory (`0x08018277`).

---

## Standard Responses and Access Control
- **Messages:** `OK`, `AT_PARAM_ERROR`, `AT_BUSY_ERROR`, `AT_NO_NET_JOINED`, `AT_ERROR`, `AT%s:`, `AT%s=` (addresses `0x080180FB–0x08018108`).
- **Password Gate:** Strings `Correct Password` / `Incorrect Password` (`0x0801810B–0x0801811E`) and prompt `Enter Password to Active AT Commands` (`0x0801759A`).
- **Debug Mode Messages:** `Use AT+DEBUG to see more debug info` (`0x080175C1`), `Exit Debug mode` (`0x08017790`).

---

## LoRaWAN / Radio Log Messages
- `RX/TX on freq ... at DR ...` (`0x080187B0–0x08018808`) + `Join Accept` / `JoinRequest NbTrials` (`0x08018828`, `0x08018887`).
- ADR tracking: `ADR Message`, `TX Datarate change to %d`, `TxPower change to %d`, `NbRep change to %d`, `Received: ADR Message` (`0x080188CA–0x08018971`).
- Timer and sync indicators: `Sync time ok`, `timestamp error`, `Set current timestamp`, `Mode for sending data for which acknowledgment was not received` (`0x08017420`, `0x08017670`, `0x08017944`).

---

## Application / Sensor Features
- Identifiers `AIS01_LB Detected`, `dragino_6601_ota`, `jpeg_flag` / `jpeg_flag2` (`0x08017366–0x080173A0`) confirm local image storage and OTA tool compatibility.
- Battery messages (`Bat_voltage:%d mv`, `No data retrieved`, `send retrieve data completed`) and power control (`PWRM2`, `Set extend the time of 5V power`).
- Remote calibration support: string `Set after calibration time or take effect after ATZ` (`0x0801763A`) and linkage to `AT+CALIBREMOTE` commands found in other analyses.

---

## Auxiliary Structures
- `analysis/ghydra_strings.csv`: 159 strings with absolute addresses → quick reference for UI and logs.
- `analysis/ghydra_pointers.csv`: 217 pointers (AT tables, state-machine jumps, radio callback vectors).
- `analysis/AU915_vectors.csv`: detailed interrupt table (Reset/ISR → `0x080147xx` for SysTick/common IRQs).
- `analysis/AU915_commands.txt`: human-readable list of 67 AT commands (including internal aliases like `+PWORD`, `+DWELLT`, etc.).

---

## Implications for New Firmware

1. **Text Replication:** Use identical messages for compatibility with existing tools and scripts (see address list for banner, errors, help, and ADR logs).  
2. **AT Parser:** Model a static table structure — command name (without `AT+`), function handler pointer, help pointer, and flags. The layout seen at `0x08016A06–0x08016A68` suggests consecutive structs of pointers.  
3. **Persistence / NVM:** Error messages like `write key error` and `Invalid credentials` confirm that configuration data is stored and validated before each operation. Implementation should ensure atomic commits and length checks.  
4. **Power Management:** Strings like `Mode for sending data for which acknowledgment was not received`, `SLEEP`, and `PWRM2` indicate distinct power states. Replicate STOP mode behavior + timers and UART gating.  
5. **LoRa Callbacks:** ADR logs and `Join Accept` logs indicate status reporting for every major radio event. The new firmware should expose equivalent hooks in `lorawan_app.c`.  
6. **Time Synchronization:** Commands `TIMESTAMP`, `SYNCMOD`, `SYNCTDC`, and `LEAPSEC` (and related strings) demonstrate RTC sync support. Design APIs to manipulate RTC registers and flash-stored timestamps.

---

## Suggested Next Steps

1. Map in Ghidra the functions referencing `AT_PARAM_ERROR`, `AT_ERROR`, and `write key error` to reconstruct the AT parser state machine and flash write flow.  
2. Build a `firmware_map.md` listing `(offset → function → role)` using exported pointers (`analysis/ghydra_pointers.csv`) and relevant strings.  
3. Design the `at_command_entry_t` structure for the new firmware, mirroring the observed triplet (name, handler, help) in `0x08016Axx`, and prepare unit tests based on collected texts.

---

✅ **Summary:**  
This document is the **central analytical reference** for understanding the structure, functionality, and persistence logic of the AIS01-LB firmware. It integrates memory mapping, AT command parsing, system logging, and calibration behavior in a single technical overview.