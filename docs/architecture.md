# Architecture

Croquettor is split into two layers:

1. **Firmware** running on an ESP32-C3 Mini
2. **Hardware** replacing the feeder's original controller board

## Functional blocks

- User input button
- Status LED
- DS3231 real-time clock
- Web configuration portal in AP mode
- Motor actuation output
- Endstop feedback input
- Non-volatile schedule storage in EEPROM emulation

## Runtime philosophy

The feeder should remain usable with no cloud dependency:

- RTC provides local timekeeping
- feeding schedules are stored locally
- AP mode is enabled only on demand
- feeding remains possible without internet access
