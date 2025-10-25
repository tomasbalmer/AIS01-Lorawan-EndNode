# Dragino AIS01-LB Custom Firmware

Custom firmware for Dragino AIS01-LB node based on STM32L072CZ and SX1276 radio, using Semtech's LoRaMAC-node stack for LoRaWAN AU915 Class A OTAA.

## Features

- **LoRaWAN Region**: AU915 (Sub-band 2)
- **Class**: Class A (OTAA)
- **MCU**: STM32L072CZ (Cortex-M0+)
- **Radio**: SX1276
- **Interface**: UART (AT commands + debug @ 115200 baud)
- **Low Power**: STOP mode + RTC (<20 µA target)
- **Remote Calibration**: Via AT commands and LoRaWAN downlinks
- **Storage**: EEPROM emulation in flash (CRC protected)

## Quick Start

### 1. Compile

```bash
make clean && make -j4
```

### 2. Flash

```bash
make flash
# or manually:
st-flash write build/dragino-ais01lb.bin 0x08004000
```

### 3. Connect Serial

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### 4. Configure

```
AT+DEVEUI=<your_deveui>
AT+APPEUI=<your_appeui>
AT+APPKEY=<your_appkey>
AT+JOIN
```

## Documentation

- **[Quick Start Guide](docs/QUICK_START.md)** - Get started in 5 minutes
- **[AT Commands Reference](docs/README.md)** - Complete AT command list
- **[Test Guide](docs/TEST_GUIDE.md)** - 12 validation tests
- **[Architecture](docs/ARCHITECTURE.md)** - Detailed architecture

## Project Structure

```
dragino-ais01lb-firmware/
├── src/
│   ├── app/              # Application code
│   ├── board/            # Board support (STM32L072 + SX1276)
│   ├── cmsis/            # CMSIS + HAL
│   ├── mac/              # LoRaMAC stack (AU915 only)
│   ├── radio/            # SX1276 driver
│   ├── system/           # System utilities
│   └── peripherals/      # Secure element
├── docs/                 # Documentation
├── Makefile              # Build system
└── stm32l072xx_flash_app.ld  # Linker script
```

## Requirements

- **Toolchain**: arm-none-eabi-gcc (10.3+)
- **Flash tool**: st-flash or STM32CubeProgrammer
- **LoRaWAN Gateway**: AU915 Sub-band 2 enabled

## AT Commands

| Command | Description | Example |
|---------|-------------|---------|
| `AT` | Test | `AT` → `OK` |
| `AT+VER` | Firmware version | `AT+VER` |
| `AT+DEVEUI=<hex>` | Set DevEUI | `AT+DEVEUI=0102030405060708` |
| `AT+APPEUI=<hex>` | Set AppEUI | `AT+APPEUI=0102030405060708` |
| `AT+APPKEY=<hex>` | Set AppKey | `AT+APPKEY=01020304...` |
| `AT+JOIN` | Join network | `AT+JOIN` |
| `AT+TDC=<ms>` | TX duty cycle | `AT+TDC=60000` |
| `AT+ADR=<0/1>` | Enable/disable ADR | `AT+ADR=1` |
| `AT+BAT` | Battery level | `AT+BAT` |
| `ATZ` | Reset device | `ATZ` |

See [docs/README.md](docs/README.md) for complete list.

## Power Consumption

| Mode | Current | Notes |
|------|---------|-------|
| TX (LoRa) | ~120 mA | During transmission |
| RUN (CPU) | ~1.5 mA | Processing |
| STOP | <20 µA | Target (RTC + flash standby) |

## License

This firmware uses Semtech's LoRaMac-node stack (Revised BSD License).

## Author

Tomas Balmer - Waterplan
- Email: tomas.balmer@waterplan.com
- GitHub: [@tomasbalmer](https://github.com/tomasbalmer)

## Version

**1.0.0** - Initial release
