# AIS01-LB Firmware (AU915) — Resumen a partir de `ghydra.txt`

## Coordenadas básicas
- **Offset de aplicación:** el volcado de Ghidra parte en `0x0000`, pero la imagen real se carga en `0x0800_4000`. Suma este desplazamiento para obtener direcciones absolutas (p. ej. `Reset` en el listado → `0x0800F30D`).
- **Tamaño de imagen:** 0x15030 bytes ≈ 86 kB, consistente con el `.bin` original.
- **Tabla de vectores:** `0x0800_4000–0x0800_40FF`. Reset → `0x0800F30D`; la mayoría de IRQs apuntan a rutinas en `0x080147xx–0x080148xx`, todas compartiendo manejadores comunes.
- **Secciones de código:** densidad alta entre `0x08004100–0x08016A00` (585 funciones reconocidas). Hay agrupaciones claras por función (HAL, LoRa, AT).
- **BSS/ZI:** tramo `0x08014900–0x0801502F` relleno con `0x00`, finaliza con palabra `0x20006D58`; refleja buffers/image caches y estructura de estado.

## Panorama de módulos (heurístico por rango)
- `0x08004100–0x08005FFF`: primitivas matemáticas/HAL (muchos `FUN_00000xxx` reutilizados). Incluye rutinas comunes (`FUN_00000148`, `FUN_0000014C`).
- `0x08006000–0x08008FFF`: controladores de periféricos (SPI/UART/RTC). Punteros hacia `0x08008770` sugieren tabla de callbacks.
- `0x08009000–0x0800DFFF`: cola de eventos LoRa y código de radio (IRQ y temporizadores).
- `0x0800F000–0x08011FFF`: capa LoRaMAC y gestión de join/uplink (`FUN_0000f28c`, `FUN_0001043c`, etc.).
- `0x08012000–0x08013FFF`: lógica de parser AT; numerosas llamadas a validadores (`FUN_000122xx`) y escritura de configuración.
- `0x08014000–0x080148FF`: bloque monolítico de tablas de texto, estado AT y rutinas Post-Join.
- `0x08014900–0x08015FFF`: buffers de trabajo (imagen JPEG, almacenamiento temporal de paquetes) y tablas de punteros.
- `0x08016A00–0x08016BFF`: tabla de comando/ayuda (punteros `0x08016A0x → 0x080080xx/0x08013xxx`).

## Tabla de comandos AT (67 entradas)
- Los nombres (`+DEUI`, `+APPKEY`, `+TDC`, `+SYNCMOD`, etc.) están serializados desde `0x0801456F` en adelante (secuencia ASCII `2B xx`); se listan íntegros en `BinAnalysis/AU915_commands.txt`.
- La tabla de punteros (`analysis/ghydra_pointers.csv`, filas `0x08016A06–0x08016A68`) enlaza con textos de ayuda ubicados en `0x08013D20–0x080183D8`.
- **Categorías clave:**
  - *LoRaWAN core:* `DEUI`, `APPEUI`, `APPKEY`, `NWKSKEY`, `APPSKEY`, `DADDR`, `ADR`, `DR`, `TXP`, `RX1DL`, `RX2DR`, `RX2FQ`, `JN1DL`, `JN2DL`, `NJM`, `CLASS`, `PNACKMD`.
  - *Regionales:* `CHE`, `CHS`, `RX1WTO/RX2WTO`, `DWELLT`, `RJTDC`, `SETMAXNBTRANS`.
  - *Sincronización & tiempo:* `TIMESTAMP`, `SYNCMOD`, `SYNCTDC`, `LEAPSEC`, `CLOCKLOG`, `RPL`, `RJTDC`.
  - *Gestión sistema:* `CFG`, `VER`, `DEBUG`, `FDR`, `PWORD`, `SLEEP`, `5VT`, `INTMOD1/2/3`.
  - *Aplicación AI:* `GETSENSORVALUE`, `PDTA`, `PLDTA`, `CLRDTA`, `UUID`, `PWRM2`.
- Validaciones relevantes se infieren de los textos: mínimo TDC de 4 s (`0x080173F9`), restricciones de sub-banda (`0x08017D6E`), advertencia post-ADR (`0x08018277`).

## Respuestas estándar y control de acceso
- **Mensajes:** `OK`, `AT_PARAM_ERROR`, `AT_BUSY_ERROR`, `AT_NO_NET_JOINED`, `AT_ERROR`, `AT%s:`, `AT%s=` (
  ubicados en `0x080180FB–0x08018108`).
- **Password gate:** cadenas `Correct Password` / `Incorrect Password` (`0x0801810B–0x0801811E`) y prompt `Enter Password to Active AT Commands` (`0x0801759A`).
- **Mensajes de modo debug:** `Use AT+DEBUG to see more debug info` (`0x080175C1`), `Exit Debug mode` (`0x08017790`).

## Log mensajes LoRaWAN/Radio
- `RX/TX on freq ... at DR ...` (`0x080187B0–0x08018808`) + `Join Accept`/`JoinRequest NbTrials` (`0x08018828`, `0x08018887`).
- Seguimiento ADR: `ADR Message`, `TX Datarate change to %d`, `TxPower change to %d`, `NbRep change to %d`, `Received: ADR Message` (`0x080188CA–0x08018971`).
- Indicadores de temporizadores y sincronización: `Sync time ok`, `timestamp error`, `Set current timestamp`, `Mode for sending data for which acknowledgment was not received` (0x08017420, 0x08017670, 0x08017944).

## Características de aplicación/sensor
- Identificadores `AIS01_LB Detected`, `dragino_6601_ota`, `jpeg_flag`/`jpeg_flag2` (`0x08017366–0x080173A0`), confirmando almacenamiento local de imágenes y compatibilidad con la herramienta OTA.
- Mensajes de batería (`Bat_voltage:%d mv`, `No data retrieved`, `send retrieve data completed`) y control de potencia (`PWRM2`, `Set extend the time of 5V power`).
- Soporte de calibración remota: cadena `Set after calibration time or take effect after ATZ` (`0x0801763A`) y conexión con comandos `AT+CALIBREMOTE` observados en otros análisis.

## Estructuras auxiliares
- `analysis/ghydra_strings.csv`: 159 strings con dirección absoluta → referencia rápida para UI y logs.
- `analysis/ghydra_pointers.csv`: 217 punteros (tablas AT, saltos de máquina de estados, vectores de callbacks de radio).
- `analysis/AU915_vectors.csv`: tabla de interrupciones detallada (Reset/ISR → 0x080147xx para SysTick/IRQ comunes).
- `analysis/AU915_commands.txt`: lista human-readable de 67 comandos AT (incluye alias internos `+PWORD`, `+DWELLT`, etc.).

## Implicaciones para el nuevo firmware
1. **Replica textual**: usa los mismos mensajes para compatibilidad con scripts/herramientas existentes (ver direcciones arriba para banner, errores, ayudas, logs ADR).
2. **Parser AT**: modela una tabla estática con: nombre (sin `AT+`), puntero a función handler, puntero a ayuda, flags. El layout visto en `0x08016A06–0x08016A68` sugiere structs consecutivos de punteros.
3. **Persistencia/NVM**: los mensajes de error `write key error`, `Invalid credentials` y las ayudas a claves confirman que la configuración se guarda y valida antes de cada operación; la implementación deberá garantizar commit atómico y checks de longitud.
4. **Gestión de energía**: interpretar `Mode for sending data for which acknowledgment was not received`, `SLEEP` y `PWRM2` como estados específicos; reproducir STOP mode + timers y gating de UARTs.
5. **LoRa callbacks**: los logs ADR y `Join Accept` indican que el firmware informa cada ajuste importante; asegúrate de exponer hooks equivalentes en `lorawan_app.c`.
6. **Sincronización temporal**: comandos `TIMESTAMP`, `SYNCMOD`, `SYNCTDC`, `LEAPSEC` y strings asociadas muestran soporte de sincronización con registros RTC; planifica API para manipular RTC y registros en flash.

## Próximos pasos sugeridos
1. Mapear en Ghidra las funciones que referencian `AT_PARAM_ERROR`, `AT_ERROR`, `write key error` para entender la máquina de estados del parser y el flujo de guardado en flash.
2. Construir un `firmware_map.md` con (offset → función → rol) usando los punteros exportados (`analysis/ghydra_pointers.csv`) y los strings relevantes.
3. Diseñar la estructura `at_command_entry_t` del nuevo firmware imitando la tríada (nombre, handler, ayuda) observada en `0x08016Axx`, y preparar pruebas unitarias con los textos recopilados.
