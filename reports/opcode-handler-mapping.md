# Agent A Report — Opcode → Handler Mapping (FUN_08001CAC)

**Summary**
- Upper-layer dispatcher = `FUN_08001CAC` (`0x08001CAC`) acting as printf-style parser.
- It loads callback pointer into `s17` (from `param_1`) before invoking `FUN_08001890` → `FUN_08001560` → `FUN_080011D4`.
- Opcode mapping is inline in the switch on the format token:

| Opcode | Handler Address | Role |
|--------|-----------------|------|
| `0x45` (`'E'`) | `0x08001560` (`FUN_08001560`) | Numeric/Exponential formatter |
| `0x47` (`'G'`) | `0x08001560` | General float formatter |
| `0x25` (`'%'`) | `DAT_08001FD4` | Literal output handler |
| Other ASCII (`0x20–0x7E`) | runtime `param_1` pointer | Delegates to caller-provided handler (UART/buffer/LoRa) |

- No static opcode table exists; handler pointers are passed dynamically from the caller context (LoRa/UART stack).
