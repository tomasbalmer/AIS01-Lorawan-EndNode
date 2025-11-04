# 🧠 AIS01-LB Firmware — Candidate Memory Access Map

This document summarizes the **absolute memory addresses** detected in the **Dragino AIS01-LB** firmware that show read/write patterns similar to **EEPROM** or **Flash** usage.  
It serves as a **preliminary reference** for tracing persistence routines, calibration data, or configuration storage behavior.

---

## 📘 EEPROM-like Regions (`0x0808xxxx`)

| Address | Count | Notes |
|----------|--------|-------|
| 0x08080808 | 6 | Frequently referenced in `Storage_Write`-like routines — likely the base of a persistent configuration block. |
| 0x08080809 | 3 | Adjacent to 0x08080808; possibly part of the same structure. |
| 0x0808090A | 3 | Recurrent pattern; could hold a CRC or “apply confirmed” flag. |
| 0x08080202 | 2 | Possible header field within a persisted structure. |
| 0x080801F9 | 1 | Single access — may indicate a version marker or structure boundary. |
| 0x08080100 | 1 | Candidate for metadata or configuration descriptor. |
| 0x08080310 | 1 | Likely calibration table entry. |
| 0x080801FD | 1 | Lower region of emulated EEPROM, rare usage. |

> 💡 These addresses fall inside the typical range of **STM32L0 emulated EEPROM (0x08080000–0x08080FFF)**, which supports the hypothesis that this block stores **persistent configuration or calibration parameters**.

---

## 🔥 Flash-like Regions (`0x0800xxxx`)

| Address | Count | Notes |
|----------|--------|-------|
| 0x0800F355 | 28 | Highly frequent call — possibly a central handler or jump table reference. |
| 0x0800ECEF | 20 | Common in control or interrupt routines. |
| 0x0800F04F | 10 | Repeated usage in initialization loops. |
| 0x0800F123 | 8 | Likely internal control function pointer. |
| 0x0800F456 | 6 | Possible state table access. |
| 0x0800F789 | 4 | May correspond to auxiliary flash function. |
| 0x0800FABC | 3 | Potential diagnostic or debug code section. |
| 0x0800FDEF | 2 | Secondary jump reference or padding filler. |
| 0x0800F012 | 2 | Early-stage initialization routine. |
| 0x0800F345 | 1 | Single-hit utility or lookup reference. |
| 0x0800F678 | 1 | Isolated call — probable inline helper. |
| 0x0800F9AB | 1 | Residual or unused vector. |
| 0x0800FCDE | 1 | Rare access; likely a constant table. |
| 0x0800F011 | 1 | Possibly related to boot code. |
