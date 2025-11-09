# AT Commands Specification — AIS01-LB

**Firmware:** v1.0.5
**LoRaWAN Stack:** DR-LWS-007 (AU915)
**Total Commands:** 67

---

## Command Categories

### LoRaWAN Network & Join Configuration (15 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+ADR** | Enable or disable Adaptive Data Rate | `0=OFF`, `1=ON` | `0x08006048` |
| **AT+APPEUI** | Set or get AppEUI (8 bytes, OTAA) | 16 hex chars | Handler table |
| **AT+APPKEY** | Set or get AppKey (16 bytes, OTAA) | 32 hex chars | Handler table |
| **AT+APPSKEY** | Set or get Application Session Key (ABP) | 32 hex chars | Handler table |
| **AT+DADDR** | Set or get Device Address (DevAddr, ABP) | 8 hex chars | Handler table |
| **AT+DEUI** | Set or get Device EUI (DevEUI, OTAA) | 16 hex chars | Handler table |
| **AT+JOIN** | Start join procedure (OTAA or ABP) | None | `0x08006030` |
| **AT+JN1DL** | Set Join Accept Delay 1 | Seconds | Handler table |
| **AT+JN2DL** | Set Join Accept Delay 2 | Seconds | Handler table |
| **AT+NJM** | Network Join Mode | `0=ABP`, `1=OTAA` | Handler table |
| **AT+NJS** | Get Join Status | Returns `0` or `1` | Handler table |
| **AT+NWKID** | Set or get Network ID | Integer | Handler table |
| **AT+NWKSKEY** | Set or get Network Session Key (ABP) | 32 hex chars | Handler table |
| **AT+CLASS** | Set or get LoRaWAN device class | `A`, `B`, `C` | Handler table |
| **AT+PNM** | Set or get public network mode | `0=Private`, `1=Public` | Handler table |

### LoRaWAN Transmission Parameters (14 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+DR** | Set or get Data Rate | `0–7` (AU915) | `0x08006054` |
| **AT+TXP** | Set or get TX Power level | `0=Max` to `5=Min` | `0x08006060` |
| **AT+PORT** | Set or get application port | `1-223` | `0x080060B4` |
| **AT+CFM** | Confirmed message mode | `0=Unconfirmed`, `1=Confirmed` | Handler table |
| **AT+DCS** | ETSI Duty Cycle setting | `0=OFF`, `1=ON` | Handler table |
| **AT+DWELLT** | Set dwell time limit | Region compliance | Handler table |
| **AT+PNACKMD** | Enable uplinks expecting ACKs | `0=OFF`, `1=ON` | Handler table |
| **AT+SETMAXNBTRANS** | Set max number of transmissions | Integer | Handler table |
| **AT+DDETECT** | Downlink detection | `0=OFF`, `1=ON` | Handler table |
| **AT+RX1DL** | RX1 delay after uplink | Seconds | Handler table |
| **AT+RX2DL** | RX2 delay after uplink | Seconds | Handler table |
| **AT+RX2DR** | RX2 data rate | `0-7` | Handler table |
| **AT+RX2FQ** | RX2 frequency | Hz | Handler table |
| **AT+RX1WTO / AT+RX2WTO** | RX1/RX2 receive timeouts | Milliseconds | Handler table |

### Counters & Security (6 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+FCU** | Get or reset uplink frame counter | Optional reset | Handler table |
| **AT+FCD** | Get or reset downlink frame counter | Optional reset | Handler table |
| **AT+DISFCNTCHECK** | Disable frame counter check (ABP test) | `0=OFF`, `1=ON` | Handler table |
| **AT+DISMACANS** | Disable automatic MAC answers | `0=OFF`, `1=ON` | Handler table |
| **AT+DECRYPT** | Enable/disable payload decryption debug | `0=OFF`, `1=ON` | Handler table |
| **AT+RJTDC** | ReJoin Timing Delay Counter | Interval value | Handler table |

### Time Synchronization & Clock (5 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+TIMESTAMP** | Set or get UNIX timestamp | UNIX epoch | `0x080061D4` |
| **AT+LEAPSEC** | Set leap second offset | Seconds | Handler table |
| **AT+SYNCMOD** | Select time sync method | `0=Disabled`, `1=NTP`, `2=LoRaWAN` | `0x080061EC` |
| **AT+SYNCTDC** | Set time sync interval | Days | Handler table |
| **AT+CLOCKLOG** | Print RTC drift and sync history | None | Handler table |

### Hardware, Sensors & Device Configuration (12 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+GETSENSORVALUE** | Read current sensor value | None | `0x08006288` |
| **AT+TDC** | Set or get Transmission Duty Cycle | Seconds (min 4s) | `0x0800603C` |
| **AT+INTMOD1/2/3** | Configure GPIO interrupt modes | Mode value | Handler table |
| **AT+5VT** | Extend 5V power supply duration | Duration | Handler table |
| **AT+SLEEP** | Enter low-power sleep mode | None | `0x08006234` |
| **AT+CFG** | Display or reset configuration | None | `0x0800606C` |
| **AT+PDTA / AT+PLDTA** | Send or preload payload data | Hex string | Handler table |
| **AT+GF** | Get or set GPIO flag | Flag value | Handler table |
| **AT+CHE / AT+CHS** | Configure channel settings/sub-band | Channel/sub-band | `0x08006180`, `0x08006174` |
| **AT+DEBUG** | Enable/disable UART debug messages | `0=OFF`, `1=ON` | `0x0800609C` |
| **AT+UUID** | Read unique MCU identifier | None | Handler table |
| **AT+CLRDTA** | Clear stored sensor data | None | Handler table |

### Diagnostic & Debug (7 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+RECV / AT+RECVB** | Inspect received downlink buffer | None / `?` for hex | Handler table |
| **AT+RSSI** | Show RSSI of last received packet | None | Handler table |
| **AT+SNR** | Show SNR of last received packet | None | Handler table |
| **AT+VER** | Display firmware version | None | `0x08006078` |
| **AT+RXDATEST** | RX window diagnostic test | None | Handler table |
| **AT+RPL** | Reload LoRa/system parameters | None | Handler table |
| **AT+BAT** | Read battery voltage | None | `0x080060A8` |

### Factory & Maintenance (4 commands)

| Command | Description | Parameters | Binary Handler |
|---------|-------------|------------|----------------|
| **AT+FDR** | Full factory reset | None | `0x08006084` |
| **AT+VER** | Display firmware version & stack info | None | `0x08006078` |
| **AT+UUID** | Display MCU unique ID | None | Handler table |
| **AT+CFG** | Show all stored configuration | None | `0x0800606C` |

---

## Standard Responses

### Success Responses

| String | Address | Usage |
|--------|---------|-------|
| `"\r\nOK\r\n"` | `0x08013222` | Generic success |
| `"Correct Password\r\n"` | `0x0801810B` | After password validation |
| `"JOINED\n\r"` | `0x08017322` | LoRaWAN join success |

### Error Responses

| String | Address | Context |
|--------|---------|---------|
| `"AT_ERROR"` | `0x0801814D` | Generic failure |
| `"AT_PARAM_ERROR"` | `0x08018130` | Parameter validation failure |
| `"AT_BUSY_ERROR"` | `0x0801811F` | Busy state |
| `"AT_NO_NET_JOINED"` | String table | Not joined to network |
| `"write config error\r\n"` | `0x080180A8` | Storage write failure |

### Informational Messages

| Message | Address | Context |
|---------|---------|---------|
| `"AIS01_LB Detected\r\n"` | `0x08017366` | Device identification |
| `"Image Version: v1.0.5\n\r"` | `0x08017404` | Firmware version |
| `"LoRaWan Stack: DR-LWS-007\n\r"` | `0x080174DD` | Stack info |
| `"Bat_voltage:%d mv"` | `0x080174B8` | Battery reading |

---

## Parser & Dispatch Architecture

### Parser Flow

```
UART RX → Ring Buffer → Parse Command → Validate Password Gate → Lookup Table → Handler
                                                                                   ↓
                                                                              Persistence
                                                                                   ↓
                                                                               Response
```

### Command Table Structure (0x08016A06)

```c
typedef struct {
    const char *name;           // Command name (without "AT+")
    ATCmdHandler_t handler;     // Function pointer to handler
    const char *help;           // Help text pointer
    uint8_t flags;              // Access flags
} ATCmdEntry_t;
```

**Table location:** `0x08016A06–0x08016A68` (67 entries)

### Validation Rules

- **Minimum TDC:** 4 seconds (`0x080173F9`)
- **Sub-band restriction:** Warning at `0x08017D6E`
- **Post-ADR advisory:** `0x08018277`
- **Password gate:** Required for write operations

### Password Protection

Strings:
- `"Enter Password to Active AT Commands"` (`0x0801759A`)
- `"Correct Password"` / `"Incorrect Password"` (`0x0801810B–0x0801811E`)

---

## Integration with LoRaWAN Core & Storage

### Handler Relationships

- **AT+JOIN** (`0x08006030`) → Invokes LoRa core join (`0x0800522C`)
- **AT+ADR/DR/TXP** → Call ADR controller (`0x08005250`) before persistence
- **AT+TDC** → Shares validator with downlink opcode `0x01`
- **AT+TIMESTAMP** → Synchronizes RTC via `0x08007024`
- **AT+GETSENSORVALUE** → Queries AI sensor over UART (`0x0800703C`)

### Storage Persistence

- **Storage module:** `0x08007030`
- **NVM base:** `0x08080808` (EEPROM shadow)
- **Validation:** Shared validators in `0x080064xx` range
- **Error handling:** `"write key error"`, `"invalid credentials"`

---

## Command Usage Notes

1. **Command Format:**
   - Query: `AT+CMD?` returns current value
   - Set: `AT+CMD=value` sets new value
   - Some commands support both forms

2. **Taking Effect:**
   - Most configuration changes require `ATZ` (reset) or `AT+JOIN` to take effect
   - Immediate effect commands: `AT+DEBUG`, `AT+GETSENSORVALUE`, `AT+SLEEP`

3. **Password Gate:**
   - Write operations require password authentication
   - Password prompt: `"Enter Password to Active AT Commands"`

4. **Debug Mode:**
   - Enable: `AT+DEBUG=1`
   - Prints: LoRaMAC events, RX/TX info, sensor data frames
   - Production: `AT+DEBUG=0` for smaller code size

5. **Compatibility:**
   - Keep response strings identical to OEM firmware for tool compatibility
   - Dragino OTA tools depend on specific message formats

---

## References

- Binary analysis: `docs/analysis/AIS01_overview.md`
- Function analysis: `docs/analysis/AIS01_function_analysis.md`
- String table: `docs/analysis/AIS01_strings.md`
- Implementation guide: `docs/implementation/AT_Handlers.md`
- Response map: Previously in `rebuild/AT_Response_Map.md`
