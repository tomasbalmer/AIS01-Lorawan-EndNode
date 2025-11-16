# Firmware Validation Harness

This folder contains structured tests designed to validate:
- uplink encoding
- downlink decoding
- NVMM behavior
- scheduler timing
- STOP-mode power cycle

Tests here provide:
- reproducible inputs
- expected outputs
- pass/fail criteria

The goal is to allow full validation **without needing hardware for every case**.
