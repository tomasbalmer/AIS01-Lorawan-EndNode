# Simulation Harness — Overview

This directory defines simulation-oriented test specifications that can be used
to verify firmware behavior **without hardware**.  
These are NOT executable scripts; they are deterministic input/output definitions
that allow reproducing firmware behavior using simple tools (CLI, Python, etc.).

Scenarios covered:
- Uplink encoding simulation
- Downlink decoding + side effects
- NVMM state transitions
- Scheduler timeline reconstruction
- STOP→WAKE→TX path

Each file describes:
- Inputs to simulate
- Expected outputs
- Pass/fail rules
- Timing or state change descriptions

---
