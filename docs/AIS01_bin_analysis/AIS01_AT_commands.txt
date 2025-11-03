# AIS01-LB — AT Command Reference  
_Firmware v1.0.5 • LoRaWAN Stack DR-LWS-007 (AU915)_

---

## 🔹 LoRaWAN Network & Join Configuration

| Command | Description |
|----------|-------------|
| **AT+ADR** | Enable or disable Adaptive Data Rate. `0=OFF`, `1=ON`. Controls automatic DR adjustment. |
| **AT+APPEUI** | Set or get the AppEUI (8 bytes, used in OTAA join). |
| **AT+APPKEY** | Set or get the AppKey (16 bytes, used in OTAA join). |
| **AT+APPSKEY** | Set or get the Application Session Key (ABP mode). |
| **AT+DADDR** | Set or get Device Address (DevAddr) for ABP mode. |
| **AT+DEUI** | Set or get Device EUI (DevEUI) used for OTAA. |
| **AT+JOIN** | Start join procedure. Executes OTAA or ABP join depending on `AT+NJM`. |
| **AT+JN1DL** | Set Join Accept Delay 1 (seconds). |
| **AT+JN2DL** | Set Join Accept Delay 2 (seconds). |
| **AT+NJM** | Network Join Mode. `0=ABP`, `1=OTAA`. |
| **AT+NJS** | Get Join Status. Returns `0` (not joined) or `1` (joined). |
| **AT+NWKID** | Set or get Network ID (LoRaWAN network identifier). |
| **AT+NWKSKEY** | Set or get Network Session Key (ABP mode). |

---

## 🔹 LoRaWAN Transmission Parameters

| Command | Description |
|----------|-------------|
| **AT+CLASS** | Set or get LoRaWAN device class (`A`, `B`, `C`). Default `A`. |
| **AT+DR** | Set or get Data Rate. Valid range `0–7` (depends on region AU915). |
| **AT+TXP** | Set or get TX Power level. `0=MaxPower` to `5=MinPower`. |
| **AT+PORT** | Set or get application port for uplinks. |
| **AT+CFM** | Confirmed message mode. `0=Unconfirmed`, `1=Confirmed`. |
| **AT+DCS** | ETSI Duty Cycle setting. `0=OFF`, `1=ON`. |
| **AT+DWELLT** | Set dwell time limit for uplinks (region compliance). |
| **AT+PNACKMD** | Enable or disable uplinks that expect ACKs (post-ACK mode). |
| **AT+SETMAXNBTRANS** | Set max number of transmissions (`NbTrans`). |
| **AT+DDETECT** | Downlink detection enable/disable. Detects missing downlinks. |
| **AT+RX1DL** | RX1 delay after uplink (seconds). |
| **AT+RX2DL** | RX2 delay after uplink (seconds). |
| **AT+RX2DR** | RX2 data rate. |
| **AT+RX2FQ** | RX2 frequency (Hz). |
| **AT+RX1WTO / AT+RX2WTO** | RX1 / RX2 receive timeouts (ms). |
| **AT+RXDATEST** | Receive window diagnostic test. Verifies radio RX path. |
| **AT+RJTDC** | ReJoin Timing Delay Counter — controls rejoin interval. |

---

## 🔹 LoRaWAN Counters & Security

| Command | Description |
|----------|-------------|
| **AT+FCU** | Get or reset uplink frame counter. |
| **AT+FCD** | Get or reset downlink frame counter. |
| **AT+DISFCNTCHECK** | Disable frame counter check (ABP test only). |
| **AT+DISMACANS** | Disable automatic MAC answers (reduce RX duty). |
| **AT+DECRYPT** | Enable or disable payload decryption in debug output. |
| **AT+PNM** | Set or get public network mode (`0=Private`, `1=Public`). |

---

## 🔹 Time Synchronization & Clock Management

| Command | Description |
|----------|-------------|
| **AT+TIMESTAMP** | Set or get UNIX timestamp. Example: `AT+TIMESTAMP=1730545054`. |
| **AT+LEAPSEC** | Set leap second offset (for GPS/RTC sync). |
| **AT+SYNCMOD** | Select time synchronization method: `0=Disabled`, `1=NTP`, `2=LoRaWAN`. |
| **AT+SYNCTDC** | Set time sync interval (days). |
| **AT+CLOCKLOG** | Print RTC drift and synchronization history. |

---

## 🔹 Hardware, Sensors & Device Configuration

| Command | Description |
|----------|-------------|
| **AT+GETSENSORVALUE** | Read current sensor value (e.g., flow, pulse, or image data). |
| **AT+TDC** | Set or get Transmission Duty Cycle (interval between sends, in seconds). |
| **AT+INTMOD1 / 2 / 3** | Configure GPIO interrupt trigger modes (PB15 / PA4 / external). |
| **AT+5VT** | Extend 5V power supply duration for sensors. |
| **AT+SLEEP** | Enter low-power sleep mode; stops LoRa and sensor activity. |
| **AT+CFG** | Display or reset configuration parameters (non-volatile memory). |
| **AT+PDTA / AT+PLDTA** | Send or preload payload data manually (uplink test). |
| **AT+GF** | Get or set GPIO flag (hardware diagnostic). |
| **AT+CHE / AT+CHS** | Configure LoRa channel settings or sub-band selection. |
| **AT+DEBUG** | Enable (`1`) or disable (`0`) UART debug messages. |
| **AT+RECV / AT+RECVB** | Inspect received downlink buffer. `AT+RECVB=?` shows hex payload. |
| **AT+RSSI** | Show RSSI of the last received LoRa packet. |
| **AT+SNR** | Show SNR of the last received LoRa packet. |
| **AT+UUID** | Read unique MCU or device identifier. |
| **AT+VER** | Display firmware version, region, and stack info. |
| **AT+FDR** | Factory reset — clears all configuration and keys. |
| **AT+CLRDTA** | Clear all stored sensor data in flash memory. |
| **AT+RPL** | Reload LoRa or system parameters (partial re-init). |

---

## 🔹 Power, Delay & Utility Commands

| Command | Description |
|----------|-------------|
| **AT+DELAYUP** | Set delay before next uplink transmission (seconds). |
| **AT+5VT** | Extend 5V output duration (sensor power). |
| **AT+PNACKMD** | Force uplinks to require ACK responses. |
| **AT+DWELLT** | Set dwell time constraint (compliance for AU915). |

---

## 🔹 Factory & Maintenance

| Command | Description |
|----------|-------------|
| **AT+FDR** | Full factory reset. Restores default keys, counters, and settings. |
| **AT+VER** | Display firmware version, LoRaWAN stack, and frequency band. |
| **AT+UUID** | Display MCU unique ID (hardware serial). |
| **AT+CFG** | Show all stored configuration parameters. |

---

## 🧩 Summary

- **Total Commands:** 67  
- **LoRaWAN configuration & join:** 15  
- **Transmission / network behavior:** 14  
- **Counters & security:** 6  
- **Time sync / RTC:** 5  
- **Sensors & hardware control:** 12  
- **Power & utility:** 4  
- **Factory / maintenance:** 4  
- **Debug / system tools:** 7  

---

### 🧠 Notes
- Most commands support both `AT+CMD?` (get) and `AT+CMD=value` (set) forms.  
- Changes often require `ATZ` (reset) or `AT+JOIN` to take effect.  
- `OK\r\n` and `AT_PARAM_ERROR` are the common responses for valid or invalid inputs.  
- The password gate (“Enter Password to Active AT Commands”) must be passed before write operations.  
- Debug mode (`AT+DEBUG=1`) prints LoRaMAC events, RX/TX info, and sensor data frames.

---
