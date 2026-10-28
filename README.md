# auto-safety-hil-ecu

A hardware-in-the-loop bench for a safety-inspired vehicle ECU. A Python 3D driving simulator talks over a real CAN bus to an STM32G474 running FreeRTOS. The ECU checks the incoming sensor data and decides whether to brake.

> **Not a production safety system.** This is an educational project. It borrows ideas from automotive functional safety (watchdogs, degraded modes, plausibility checks, deterministic state machines). It does not claim ISO 26262 or MISRA compliance, and it controls only a simulated vehicle.

## License
MIT, see [LICENSE](LICENSE). Third-party code (STM32Cube HAL, CMSIS, FreeRTOS, FatFs, Unity) keeps its own license.
