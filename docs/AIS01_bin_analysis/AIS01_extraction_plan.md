# AIS01 Binary Reverse Engineering – Implementation Plan

Este plan resume lo que ya tenemos documentado a partir del binario original y lo que falta extraer antes de rehacer el firmware.

## 1. Artefactos existentes
| Categoría | Archivo | Contenido |
|-----------|---------|-----------|
| Vectores de interrupción | `AIS01_vectors.csv` | Vector table completo con handlers y direcciones. |
| Cadenas de texto | `AIS01_strings.csv` | Strings de AT, logs y mensajes de estado. |
| Punteros globales | `AIS01_pointers.csv` | Referencias a tablas, despachadores y callbacks. |
| Funciones (lista) | `AIS01_functions.csv` | Dirección, nombre provisional y tamaño de cada rutina. |
| Funciones (análisis) | `AIS01_function_analysis.md` | Descripción de funciones clave ya revisadas. |
| Comandos AT | `AIS01_AT_commands.txt` | Sintaxis y handlers asociados encontrados en el decompilado. |
| Candidatos de dirección | `AIS01_address_candidates.txt` | Direcciones sospechosas ligadas a periféricos/config. |
| Mapa NVM | `AIS01_nvm_map.txt` | Regiones de flash usadas para configuración y claves. |
| Resumen general | `AIS01_overview.md` | Hallazgos iniciales y notas de reverse. |

## 2. Pendientes de extracción
| Prioridad | Tópico | Objetivo | Sugerencia de salida |
|-----------|--------|----------|----------------------|
| Alta | Pinout y periféricos | Mapear GPIO ↔ SX1276, UARTs, sensores, reguladores. | `AIS01_pinout.md` |
| Alta | Secuencia de arranque / reloj | Detallar SystemInit, RCC, RTC, watchdog, fuentes de reloj. | `AIS01_clock_boot.md` |
| Alta | Máquina de estados principal | Documentar flujo BOOT→JOIN→TX/RX→SLEEP y timers asociados. | `AIS01_state_machine.md` |
| Alta | Tablas LoRaWAN AU915 | Extraer arrays de canales, DR, potencias, delays. | `AIS01_lorawan_tables.md` |
| Media | Dispatcher de downlinks | Identificar tabla opcode→handler, payloads y respuestas. | `AIS01_downlink_dispatch.md` |
| Media | Rutinas de persistencia flash | Describir acceso a NVM, estructuras, factory reset. | `AIS01_storage.md` |
| Media | Gestión de energía | Secuencia de entrada/salida de STOP, periféricos que se apagan. | `AIS01_power.md` |
| Media | Interrupciones detalladas | Analizar ISR y su relación con colas/eventos. | `AIS01_isr_notes.md` |
| Baja | RNG / entropía | Fuente de devnonce/random y validaciones. | `AIS01_rng.md` |
| Baja | Protocolo sensor AI | Interacción con segundo UART/I²C/SPI y calibración original. | `AIS01_sensor_interface.md` |

## 3. Próximos pasos 
1. Priorizar pinout y relojes porque impactan directo en el port sobre LoRaMAC-node.
2. A medida que se analicen funciones, actualizar los archivos de la sección 2 con hallazgos verificables.
3. Mantener este plan vivo: mover ítems completados a la lista de artefactos existentes y añadir nuevas necesidades conforme aparezcan.

