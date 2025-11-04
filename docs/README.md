# Dragino AIS01-LB – Resumen Técnico

Firmware personalizado para el nodo Dragino AIS01-LB (STM32L072CZ + SX1276), basado en la pila LoRaMAC-node de Semtech, con capa de drivers propia, comandos AT estilo Dragino y motor de calibración remota. El objetivo es reemplazar el firmware OEM manteniendo la compatibilidad con OTAA Clase A en AU915 Sub-band 2 y optimizando el consumo en STOP mode (<20 µA).

## Compilación

### Requisitos

- `arm-none-eabi-gcc` 10.3 o superior (probado con Arm GNU Toolchain 14.3 rel1)
- GNU Make

### Pasos

```bash
make clean
make -j4
```

Artefactos generados en `build/`:

- `ais01.elf`  – imagen con símbolos para depuración
- `ais01.hex`  – formato Intel HEX
- `ais01.bin`  – binario listo para carga (offset 0x08004000)

## Carga del firmware

### Dragino OTA Tool

1. Abrir la herramienta OTA de Dragino
2. Seleccionar `build/ais01.bin`
3. Configurar dirección de aplicación `0x08004000`
4. Iniciar la transferencia por UART según el manual de Dragino

### Programación por SWD

```bash
make flash
# o manualmente
st-flash write build/ais01.bin 0x08004000

STM32_Programmer_CLI -c port=SWD -w build/ais01.bin 0x08004000 -v -rst
```

> **Importante:** el bootloader OEM ocupa las primeras 16 KB; la aplicación debe iniciar en `0x08004000`.

## Comandos AT

Listado reducido. El detalle completo está en `docs/rebuild/AT_Handlers.md`.

### Básicos

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT` | Ping del parser | `AT` → `OK` |
| `AT+VER` | Versión firmware | `AT+VER` |
| `ATZ` | Reboot suave | `ATZ` |
| `AT+FDR` | Factory reset | `AT+FDR` |

### Credenciales LoRaWAN

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `AT+DEVEUI` | Get/Set DevEUI | `AT+DEVEUI=0102030405060708` |
| `AT+APPEUI` | Get/Set AppEUI | `AT+APPEUI=0102030405060708` |
| `AT+APPKEY` | Get/Set AppKey | `AT+APPKEY=001122...` |
| `AT+JOIN` | Iniciar OTAA join | `AT+JOIN` |

### Configuración de enlace

| Comando | Descripción | Valores |
|---------|-------------|---------|
| `AT+ADR=<0/1>` | ADR enable | 0 (OFF), 1 (ON) |
| `AT+DR=<n>` | Data rate | 0–5 (AU915) |
| `AT+TXP=<n>` | Potencia TX | 0–10 |
| `AT+TDC=<ms>` | Intervalo de TX | ≥1000 ms |
| `AT+PORT=<n>` | Puerto de aplicación | 1–223 |
| `AT+PNACKMD=<0/1>` | Confirmed/unconfirmed | 0 = Unconfirmed |
| `AT+RX2DR=<n>` | RX2 data rate | DR_8 por defecto |
| `AT+RX2FQ=<Hz>` | RX2 frequency | 923300000 |
| `AT+FREQBAND=<n>` | Sub-band AU915 | 1–9 (usar 2) |

### Sistema y calibración

| Comando | Descripción |
|---------|-------------|
| `AT+BAT` | Medición de batería |
| `AT+CFG` | Imprime configuración actual |
| `AT+DEBUG=<0/1>` | Activa/desactiva logs UART |
| `AT+CALIBREMOTE=<hex>` | Calibración remota (aplica/query) |

La semántica exacta de los payloads de calibración y de los opcodes downlink se detalla en:

- `docs/rebuild/Calibration_Engine.md`
- `docs/rebuild/Downlink_Dispatcher.md`

## Flujo de uso recomendado

1. Flashear `build/ais01.bin` manteniendo el bootloader OEM.
2. Conectar UART a 115200 8N1 y enviar `AT` para validar la consola.
3. Configurar DevEUI/AppEUI/AppKey y ejecutar `AT+JOIN`.
4. Ajustar `AT+TDC`, `AT+ADR`, `AT+PORT` según la aplicación.
5. Validar calibración remota con `AT+CALIBREMOTE=<payload>` y downlink opcode `0xA0`.
6. Seguir el plan de pruebas (`docs/rebuild/Test_Plan.md`) para validar AT, downlinks y consumo en STOP.

## Documentación relacionada

- `docs/rebuild/Firmware_Architecture_Map.md` – capas y módulos
- `docs/rebuild/Hardware_Power.md` – estrategia de bajo consumo
- `docs/rebuild/Scheduler.md` – máquina de estados y timers
- `docs/rebuild/Test_Plan.md` – plan de validación en hardware
- `docs/AIS01_bin_*` – notas de ingeniería inversa del firmware OEM

## Estado actual

- Binario generado: `build/ais01.bin` (~50 KB)
- Calibración persistida en RAM (misma conducta OEM)
- Pendiente validar en banco: plan de tests completo y medición de corriente en STOP
