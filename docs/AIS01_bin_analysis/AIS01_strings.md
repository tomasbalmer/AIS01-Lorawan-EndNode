# Dragino AIS01-LB — Extracted Firmware Strings (Annotated Reference)

**Source:** `AIS01-LB_v1.0.5.bin`  
**Extracted from:** `.rodata` section (`0x0801721F – 0x08018348`)  
**Toolchain:** Ghidra 11.4.2  
**Purpose:** This document replaces the raw CSV dump of firmware strings.  
It provides categorized, human-readable insight into the device’s AT command set, internal logs, and LoRaWAN stack messages.  
All strings are mapped to their binary offsets for traceability.

---

## 1. Overview

The Dragino **AIS01-LB** is a LoRaWAN image node (STM32L072 + SX1276).  
This list represents all text constants embedded in firmware **v1.0.5**, used across:

- UART **AT command interface**  
- **System logs and debug messages**  
- **LoRaWAN stack** (Semtech-based)  
- **Camera / JPEG subsystem**  
- **Time synchronization and calibration**

Each entry includes:
- **Address** — exact memory offset (useful in Ghidra)
- **String**
- **Category**
- **Notes / Context** — when its function can be inferred

---

## 2. Firmware String Map

| Address | String | Category | Notes / Context |
|:---:|:---|:---|:---|
| 0x0801721F | `AT+NAME%02X%02X%02X%02X%02X%02X%02X%02X` | AT Command / Format | Format template for `AT+NAME` response. |
| 0x08017249 | `Bat_voltage:%d mv` | System Log | Battery voltage in millivolts. |
| 0x0801725F | `erase all sensor data storage error` | Error | Flash erase failure. |
| 0x0801728A | `write config error` | Error | Error writing configuration to memory. |
| 0x080172D3 | `system_reset` | System | Device reset notification. |
| 0x080172E0 | `jpeg_size:%d` | Camera | JPEG image size before LoRa transfer. |
| 0x080172ED | `The image data is too large, stop sending` | Camera / Error | Oversized JPEG buffer stopped. |
| 0x0801730E | `switch to class %c done` | LoRaWAN | Confirms LoRa class change (A/B/C). |
| 0x08017322 | `JOINED` | LoRaWAN | Successful network join (OTAA/ABP). |
| 0x0801732A | `AT+PWRM2` | AT Command | Power-mode command. |
| 0x08017358 | `dragino_6601_ota` | OTA / Bootloader | Identifier string for OTA bootloader recognition. |
| 0x0801736A | `AIS01_LB Detected` | Boot Log | Hardware detected at startup. |
| 0x0801737C | `Bat:%.3f V` | System Log | Battery voltage (volts). |
| 0x08017387 | `send retrieve data completed` | System | End of data retransmission. |
| 0x080173B0 | `TDC setting needs to be high than 4s` | Validation | Minimum delay constraint. |
| 0x080173D0 | `Set current timestamp=%u` | Time Sync | Timestamp set confirmation. |
| 0x080173E8 | `timestamp error` | Error | Invalid timestamp or sync failure. |
| 0x0801740A | `BuffSize:%d,Run AT+RECVB=? to see detail` | AT Help | Buffer info prompt. |
| 0x0801742A | `AU915` | Region | Band code for AU915 region. |
| 0x08017436 | `DevEui= %02X ...` | LoRaWAN | Printed device EUI. |
| 0x0801747E | `v1.0.5` | Version | Firmware version label. |
| 0x08017485 | `Invalid credentials,the device goes into low power mode` | Security | Authentication failure response. |
| 0x080174B0 | `Dragino AIS01-LB Device` | Boot ID | Product identifier. |
| 0x080174DD | `LoRaWan Stack: DR-LWS-007` | LoRaWAN Stack | Semtech stack version. |
| 0x08017500 | `Enter Password to Active AT Commands` | AT Auth | Password-protected AT mode. |
| 0x0801751E | `Use AT+DEBUG to see more debug info` | AT Help | Debug hint. |
| 0x08017575 | `Set after calibration time or take effect after ATZ` | Calibration | Parameter activation note. |
| 0x0801759A | `Mode for sending data for which acknowledgment was not received` | LoRaWAN | No-ACK retransmission policy. |
| 0x080175C1 | `Stop Tx events when read sensor data` | Scheduler | Suspend TX when sampling sensors. |
| 0x080175F0 | `Set password,2 to 8 bytes` | AT Help | `AT+PWORD` length limit. |
| 0x0801760A | `Get current image version and Frequency Band` | AT Help | `AT+VER` output. |
| 0x0801766A | `Enter Debug mode` | Debug | Enable extended debug output. |
| 0x0801768A | `Get or set the MAC ANS switch(0:Enable,1:Disable)` | LoRaWAN Config | MAC answer control. |
| 0x0801770A | `Get or Set extend the time of 5V power` | Camera / Power | Keeps 5V active for camera. |
| 0x0801772A | `Get or set time synchronization interval in day` | Time Sync | Periodic time sync interval. |
| 0x0801774A | `Get or Set time synchronization method` | Time Sync | NTP/LoRaWAN sync selection. |
| 0x080177AA | `Get or set the ReJoin data transmission interval in min` | LoRaWAN | Rejoin scheduling. |
| 0x080177EA | `Get or Set the Decrypt the uplink payload(0:Disable,1:Enable)` | Security | Payload decryption flag. |
| 0x0801786A | `Get or set the application data transmission interval in ms` | LoRaWAN / App | TX interval setting. |
| 0x0801788A | `Set the delay in seconds before sending packets (in seconds)` | LoRaWAN / App | Pre-TX delay. |
| 0x080178AA | `Get current sensor value` | Sensor | `AT+GETSENSORVALUE`. |
| 0x080178D5 | `Get the device Unique ID` | System | `AT+UUID` output. |
| 0x080178EA | `Stop Tx events,Please wait for the erase to complete` | System | Flash erase blocking. |
| 0x0801790A | `Clear all stored sensor data...` | Storage | Start of flash erase. |
| 0x0801794A | `Set sleep mode` | Power | `AT+SLEEP` command. |
| 0x0801796A | `Get or Set the trigger interrupt mode of PB15(...)` | GPIO Config | Configurable interrupt mode. |
| 0x080179CA | `systime= %d/%d/%d %02d:%02d:%02d (%u)` | Time Sync | Current system time print. |
| 0x080179EA | `Get or Set UNIX timestamp in second` | Time Sync | `AT+TIMESTAMP`. |
| 0x08017A0A | `Error Subband, must be 0 ~ 8` | LoRaWAN / Validation | AU915 sub-band validation. |
| 0x08017A4A | `Get or Set eight channels mode,Only for US915,AU915,CN470` | LoRaWAN Config | 8-channel restriction. |
| 0x08017A8A | `Get or set the application port` | LoRaWAN Config | `AT+PORT`. |
| 0x08017AAA | `Get the RSSI of the last received packet` | LoRaWAN Info | `AT+RSSI`. |
| 0x08017ACA | `Get the SNR of the last received packet` | LoRaWAN Info | `AT+SNR`. |
| 0x08017B2A | `Join network` | LoRaWAN | `AT+JOIN`. |
| 0x08017B4A | `Get or Set the Device Class` | LoRaWAN Config | `AT+CLASS`. |
| 0x08017C0A | `Get or Set the delay between the end of the Tx and the Rx Window 2 in ms` | LoRaWAN Timing | RX2 delay. |
| 0x08017C6A | `Get or Set the Rx2 window frequency` | LoRaWAN Config | RX2 frequency. |
| 0x08017CAA | `Get or Set the Transmit Power (0-5, MAX:0, MIN:5)` | LoRaWAN Config | `AT+TXP`. |
| 0x08017CCA | `Get or Set the Adaptive Data Rate setting. (0: off, 1: on)` | LoRaWAN Config | ADR toggle. |
| 0x08017CEA | `Get or Set the Network Join Mode. (0: ABP, 1: OTAA)` | LoRaWAN Config | Join mode. |
| 0x08017D0A | `Get or Set the ETSI Duty Cycle setting - 0=disable, 1=enable` | LoRaWAN / Test | Duty cycle test mode. |
| 0x08017D8A | `Get or Set the Network Session Key` | LoRaWAN Keys | `AT+NWKSKEY`. |
| 0x08017DAA | `Get or Set the Application Session Key` | LoRaWAN Keys | `AT+APPSKEY`. |
| 0x08017DCA | `Get or Set the Device Address` | LoRaWAN Keys | `AT+DADDR`. |
| 0x08017DEA | `Get or Set the Application Key` | LoRaWAN Keys | `AT+APPKEY`. |
| 0x08017E0A | `Get or Set the Application EUI` | LoRaWAN Keys | `AT+APPEUI`. |
| 0x08017E2A | `Get or Set the Device EUI` | LoRaWAN Keys | `AT+DEUI`. |
| 0x08017E5A | `Reset Parameters to Factory Default, Keys Reserve` | System | Factory default reset. |
| 0x08017E7A | `Trig a reset of the MCU` | System | MCU reset command. |
| 0x08017F08 | `AT_ERROR` | AT Response | Generic error. |
| 0x08017F18 | `Correct Password` | AT Response | Password accepted. |
| 0x08017F28 | `Incorrect Password` | AT Response | Authentication failure. |
| 0x08017F3A | `AT_PARAM_ERROR` | AT Response | Parameter invalid. |
| 0x08017F4A | `AT_BUSY_ERROR` | AT Response | Device busy. |
| 0x08017F5A | `AT_NO_NET_JOINED` | AT / LoRaWAN | Network not joined. |
| 0x08017F6A | `+DEBUG` | AT Token | Debug. |
| 0x08017F72 | `+FDR` | AT Token | Factory reset. |
| 0x08017F78 | `+DEUI` | AT Token | Device EUI. |
| 0x08018014 | `+JOIN` | AT Token | Join command. |
| 0x0801802C | `+VER` | AT Token | Version query. |
| 0x08018054 | `+PORT` | AT Token | Application port. |
| 0x0801805C | `+PWORD` | AT Token | Password. |
| 0x080180CC | `+5VT` | AT Token | 5 V power hold (camera). |
| 0x080180EC | `+SLEEP` | AT Token | Sleep control. |
| 0x08018112 | `+UUID` | AT Token | Unique ID. |
| 0x0801812A | `+GETSENSORVALUE` | AT Token | Read sensor values. |
| 0x08018162 | `txDone` | LoRaWAN Event | Transmission complete. |
| 0x0801816A | `rxDone(ACK)` | LoRaWAN Event | ACK received. |
| 0x08018184 | `rxError` | LoRaWAN Event | Reception error. |
| 0x0801818C | `txTimeout` | LoRaWAN Event | Transmission timeout. |
| 0x0801819C | `RX on freq %u Hz at DR %d` | LoRaWAN Log | Radio RX info. |
| 0x080181EC | `TX on freq %d.%d MHz at DR %d` | LoRaWAN Log | Radio TX info. |
| 0x0801820C | `Join Accept:` | LoRaWAN Log | Join acceptance. |
| 0x08018258 | `JoinRequest NbTrials= %d` | LoRaWAN Log | Join attempts counter. |
| 0x08018274 | `***** UpLinkCounter= %u *****` | LoRaWAN Debug | Uplink counter display. |
| 0x0801828C | `ADR Message:` | LoRaWAN ADR | Adaptive Data Rate message. |
| 0x080182AC | `TX Datarate change to %d` | LoRaWAN ADR | Data rate update log. |
| 0x080182C0 | `TxPower change to %d` | LoRaWAN ADR | TX power change. |
| 0x080182E0 | `TX Datarate too small for this payload size, change to %d` | LoRaWAN ADR | Payload adjustment warning. |
| 0x08018318 | `Sync time ok` | Time Sync | Successful synchronization. |
| 0x08018340 | `AAAAAA` | Debug filler | Unused test string. |
| 0x08018348 | `BBBBBB` | Debug filler | Unused test string. |

---

## 3. AT Command Index

| Command | Purpose |
|----------|----------|
| `AT+NAME` | Set or display device name. |
| `AT+PWRM2` | Power mode control. |
| `AT+VER` | Firmware version and frequency band. |
| `AT+DEBUG` | Enable or disable verbose mode. |
| `AT+PWORD` | Set AT command password (2–8 bytes). |
| `AT+5VT` | Extend 5 V supply time for camera module. |
| `AT+TIMESTAMP` | Get/Set UNIX timestamp. |
| `AT+SLEEP` | Enter low-power mode. |
| `AT+UUID` | Retrieve unique MCU ID. |
| `AT+GETSENSORVALUE` | Read current sensor measurement. |
| `AT+PLDTA` / `AT+PDTA` | Print payload / data logs. |
| `AT+CLRDTA` | Clear stored sensor data. |
| `AT+JOIN` | Start LoRaWAN join procedure. |
| `AT+CLASS` | Get/Set LoRaWAN device class. |
| `AT+RX1DL`, `AT+RX2DL`, `AT+JN1DL`, `AT+JN2DL` | Configure receive and join window delays. |
| `AT+ADR`, `AT+TXP`, `AT+DWELLT`, `AT+RPL` | ADR, transmit power, dwell, repeat parameters. |
| `AT+DECRYPT` | Toggle payload decryption. |
| `AT+SETMAXNBTRANS` | Set max retransmission count. |
| `AT+FDR` | Reset to factory defaults (keep keys). |
| `AT+RECVB` | Print last received frame in binary. |

---

## 4. Critical Firmware Strings

| String | Importance |
|---------|-------------|
| `dragino_6601_ota` | Identifies OTA bootloader family; required for OTA compatibility. |
| `Image Version: v1.0.5` | Image header version, appears in AT+VER and boot logs. |
| `Set current timestamp=%u` / `timestamp error` | Time sync handler; related to “1970” timestamp issue. |
| `Get or Set extend the time of 5V power` / `+5VT` | Controls camera 5 V rail duration — key for calibration. |
| `Stop Tx events,Please wait for the erase to complete` | Blocking flash erase; scheduler pauses TX. |
| `Clear all stored sensor data...` | Flash erase operation entry point. |

---

## 5. Metadata

```text
Firmware: Dragino AIS01-LB v1.0.5  
MCU: STM32L072CZ  
Region: AU915  
Stack: LoRaMAC-Node DR-LWS-007  
Extraction: Ghidra Strings window
Analysis context: Reverse-engineering and documentation of the original firmware for custom LoRaWAN Class A implementation.  