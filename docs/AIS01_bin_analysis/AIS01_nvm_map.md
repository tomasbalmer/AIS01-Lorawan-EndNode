# AIS01-LB — Non-Volatile Memory Map

**Address Range:** 0x08080000–0x08081FFF  
**MCU:** STM32L072CZ (EEPROM emulated in Flash)

---

## Candidate Base Addresses

| Address | Frequency | Notes |
|----------|------------|-------|
| 0x08080808 | 6 | Likely configuration block header |
| 0x08080809 | 3 | Possible sub-field or flag |
| 0x0808090A | 3 | Timestamp or log entry |
| 0x08080202 | 2 | AT parameters |
| 0x080801F9 | 1 | Flag FDR |
| 0x08080100 | 1 | Magic/version header |
| 0x08080310 | 1 | Calibration slot |
| 0x080801FD | 1 | SyncMod or counter |

---

## Probable Stored Fields

- **magic/version** — EEPROM block identifier  
- **SYNCMOD (u8)** — Synchronization mode  
- **TIMESTAMP (u32)** — Last sync time or operation timestamp  
- **Flags FDR** — Factory reset / calibration reset indicator  
- **AT Parameters** — User-configurable AT command settings  

---

## Flash Reference (for context)

| Address | Frequency | Description |
|----------|------------|-------------|
| 0x0800F355 | 28 | Main handler reference |
| 0x0800ECEF | 20 | Secondary handler |
| 0x0800F04F | 10 | LoRa or AT command code |
| 0x0800F123 | 8 | Data block |
| 0x0800F456 | 6 | Helper routine |

---

### Notes

All addresses inside `0x08080000–0x08081FFF` correspond to non-volatile (EEPROM-emulated) storage.  
Higher addresses (`0x0800Fxxx–0x08014xxx`) are regular Flash containing code and constant strings.