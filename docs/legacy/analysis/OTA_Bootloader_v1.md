LoRa OTA Bootloader v1.4 (Dragino) — Bootloader Analysis

0. Purpose

Living document containing all extracted evidence from the binary LoRa_OTA_Bootloader_v1.4.bin (slice 0x0..0xD000). Goal: preserve all technical findings from the bootloader so that the application team can design interoperability and test procedures.

⸻

1. Quick Summary
	•	Type: Dragino OTA Bootloader v1.4 (multi-region variants: AS923, AU915, US915, EU868, etc.).
	•	Analyzed slice size: ~0xD000 bytes (≈53 KB). In most sessions, a 0x9000 block was used for disassembly and sub-slices were extracted as needed.
	•	Vector table detected at 0x08000000.
	•	Initial Stack Pointer: 0x2000F000.
	•	Reset handler (PC) detected at 0x08002149 (aligned to 0x08002148).
	•	Early functions configure NVIC/SysTick/MMIO and copy data from Flash to RAM (likely data relocation/init).
	•	Contains AT command strings (AT+DATA=, AT_ERROR, +FREQ, etc.) and version strings such as "Dragino OTA bootloader ... v1.4" for multiple frequency bands (AU915 included).

⸻

2. Memory Map (verified portions)
	•	FLASH (defined in Ghidra): 0x08000000 — 0x0800CFFF (expanded up to 0xD000 in some sessions). Locally mapped as LoRa_OTA_Bootloader_v1.4.bin[0x0,0x9000].
	•	RAM (from vector table): 0x2000F000 (initial stack pointer).
	•	Reset handler file offset: reset_addr & ~1 = 0x08002148 → file_offset = 0x2148 in the binary.

⸻

3. Vector Table (relevant entries)

Base vector table: 0x08000000 — 48 entries.
	•	[0] SP: 0x2000F000
	•	[1] Reset: 0x08002148 (Thumb) — main entry of the bootloader
	•	Example IRQ vectors: 0x080073C8, 0x080073CA, 0x080073CC, 0x080073CE, etc.
	•	Many entries point to the same handler 0x08002190 (likely a generic IRQ handler or jump table dispatcher).

(The full table appears in previous logs and extracted listings.)

⸻

4. Identified Functions / Blocks (addresses and inferred roles)

Note: Function names are heuristic; addresses and observed behaviors are preserved.

	•	0x08002148 — Reset handler (copy/relocates data, calls inits, loops indefinitely):
	•	Copies tables to RAM (ldr/str pairs), compares with limits at 0x2000046C, 0x20000470, 0x20001488.
	•	Calls FUN_08002104, FUN_08007A70, FUN_080072B0.
	•	Ends in b . (infinite loop) — expected for a bootloader awaiting AT or OTA commands.
	•	0x08002104 — NVIC / Peripheral Init
	•	Writes to E000ED00 and E000E100 (System Control / NVIC configuration).
	•	Accesses 0x4000000C and 0x40020014 (likely GPIO/RCC/SYSCFG base addresses).
	•	Calls FUN_080020E0 (also accesses MMIO at E000ED00/E000E100 and writes bytes at large offsets → interrupt setup logic).
	•	0x080020E0 — Writes to E000ED22, loops, and writes to memory offset #0x300 from computed base (low-level SCB/NVIC manipulation).
	•	0x08007A70, 0x080072B0 — Post-init routines; possibly radio/UART/scheduler setup or transition to main loop.
	•	0x08002190 — Referenced by many IRQ vectors (common handler). A descriptor table at 0x0800217C points to 0x08008BDC (likely secondary jump/data table in Flash).

⸻

5. AT Command Strings (key evidence)

strings scan of the binary returned:
	•	AT+DATA=
	•	AT_ERROR
	•	+FREQ, +MOD
	•	Version identifiers: Dragino OTA bootloader AS923 v1.4, ... AU915 v1.4, ... US915 v1.4, ... EU868 v1.4, etc.
	•	Debug hooks: OnTxTimeout, OnRxTimeout, OnRxError, AT_PARAM_ERROR, AT_BUSY_ERROR.

✅ Confirms that the bootloader implements an AT command parser and multi-band configuration.

⸻

6. MMIO / Peripheral Access (observed through disassembly)
	•	E000ED00, E000E100, E000ED88 — System Control Block / NVIC / SysTick.
	•	0x4000000C — Peripheral register (possible SYSCFG/EXTI/RCC depending on MCU).
	•	0x40020014 — Likely GPIO port configuration register.

🧩 Conclusion: Bootloader configures NVIC, system control, and GPIO peripherals.

⸻

7. Watchdog and WFI Observations
	•	Heuristic scan found no explicit IWDG signatures (0xAAAA/0xCCCC patterns) or WFI instructions.
	•	This does not exclude watchdog use; initialization could be indirect via HAL or embedded elsewhere.
	•	The reset/init routines show NVIC/SysTick activity but no direct writes to IWDG registers.

⸻

8. Implications for Application Firmware
	•	The bootloader manages low-level hardware (NVIC/GPIO) and exposes AT commands.
	•	The application must respect Flash boundaries and avoid overwriting the bootloader vector table.
	•	The bootloader relocates data into 0x2000xxxx; the app must avoid RAM collisions.
	•	Knowing the bootloader vector table helps:
	•	Identify which IRQs it uses.
	•	Avoid handler conflicts in application firmware.

⸻

9. Useful Commands (for reproducibility)
	1.	Hexdump + Header (vector and reset):

F=~/Downloads/LoRa_OTA_Bootloader_v1.4.bin
hexdump -C "$F" | sed -n '1,40p'
od -An -v -t x4 -N 8 "$F"

	2.	SP/Reset check and reset handler dump:

python3 - <<'PY'
import struct,os
F=os.path.expanduser('~/Downloads/LoRa_OTA_Bootloader_v1.4.bin')
with open(F,'rb') as fh:
    head=fh.read(8)
    sp,pc=struct.unpack_from('<II', head, 0)
    reset_addr = pc & ~1
    file_offset = reset_addr - 0x08000000
    print(hex(sp), hex(pc), hex(file_offset))
PY
xxd -g1 -s 0x2148 -l 512 "$F" | sed -n '1,40p'

	3.	Extract and disassemble reset slice:

OFF=0x2148; LEN=0x2000
dd if=LoRa_OTA_Bootloader_v1.4.bin of=bl_reset.bin bs=1 skip=$OFF count=$LEN status=none
arm-none-eabi-objdump -D -b binary -marm -M force-thumb --adjust-vma=0x08002148 bl_reset.bin | sed -n '1,200p'

	4.	Strings (search AT/boot/version identifiers):

strings LoRa_OTA_Bootloader_v1.4.bin | egrep -i "AT\+|Dragino|bootloader|OTA|Image Version|OnTxTimeout|OnRxTimeout" -n

	5.	Full disassembly of .text:

arm-none-eabi-objdump -D -b binary -marm -M force-thumb --adjust-vma=0x08000000 LoRa_OTA_Bootloader_v1.4.bin > bl_full.disasm.txt


⸻

10. Next Recommended Steps
	1.	In Ghidra, ensure proper code unit creation: at 0x08002148, define the function manually if the script fails.
	2.	Extract AT string references and XREFs to locate AT command handler functions.
	3.	Dump table at 0x08008BDC (likely image info or jump table).
	4.	Scan for IWDG register access (0x40003000 range) or writes to IWDG_KR/PR/RLR if watchdog validation is needed.
	5.	Finalize this artifact with attached outputs (disassembly, strings, hexdumps).

⸻

11. Artifact Completion Checklist
	•	Vector table + SP/Reset confirmed.
	•	Reset handler disassembly extracted.
	•	AT / boot ID strings verified.
	•	MMIO / NVIC evidence confirmed.
	•	AT handler cross-references mapped.
	•	IWDG presence confirmed (TBD).
	•	RAM regions reserved by bootloader mapped.

⸻

12. Next Iteration (what to collect next)
	•	Paste full objdump of bl_reset.bin slice (first 200–400 lines).
	•	Paste strings output filtered by AT/Dragino keywords.
	•	Provide output of vector-to-function creation script (if any differences appear).

⸻
