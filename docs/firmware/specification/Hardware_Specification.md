# Hardware Specification  
## AIS01-LB Custom Firmware

This document describes ONLY the hardware elements relevant  
to the custom firmware implementation — NOT the OEM behavior.

---

# 1. MCU

STM32L072CZ  
- 192 KB flash  
- 20 KB SRAM  
- STOP mode support  
- IWDG watchdog  

Clocks:  
- MSI default  
- LSE for RTC (if populated)  

---

# 2. Radio (SX1276)

Interface:  
- SPI  
- DIO0 = Rx/Tx done  
- DIO1 = CAD  
- RESET pin  
- regulator (3.3V) controlled via GPIO  

Radio is powered OFF in STOP mode.

---

# 3. Power Rails

- MCU: 3.3 V  
- SX1276: 3.3 V regulated  
- Optional camera 5V rail (if present)  

Regulator enable pin is controlled in `board/` layer.

---

# 4. GPIO Summary

| Name | Function |
|------|----------|
| PA0 | RTC wake / general input |
| PA4 | SX1276 NSS |
| PB3 | SX1276 DIO0 |
| PB4 | SX1276 DIO1 |
| PB5 | SX1276 RESET |
| PC13 | Regulator EN |

(*Pin mapping adjusted to match board drivers*)

---

# 5. UART

UART1 @ 115200 used for:  
- AT commands  
- debug  

---

# 6. Flash Layout

Flash pages used for NVMM (EEPROM emulation):  
- dedicated region defined in `storage.c`  
- CRC32 verified on boot  

---

# Source of Truth
Matches `board/`, `radio/`, and `storage/` implementations.
