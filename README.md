# Croquettor

Open-source retrofit of a commercial automatic cat feeder using an **ESP32-C3 Mini** and a **DS3231 RTC**.

Croquettor replaces the original controller board of a dry-food dispenser with a reproducible, repairable, and fully hackable electronics + firmware stack.

> Tested / targeted feeder platform: **Balimo Automatic Cat Feeder (2.4G WiFi)**.
>
> This mention is provided **for compatibility and identification only**. This project is **independent**, **unofficial**, and **not affiliated with or endorsed by** the original manufacturer.

---

## Why this project exists

Many connected pet feeders rely on proprietary boards, cloud dependencies, and limited repairability. Croquettor aims to:

- replace the original motherboard with an open design,
- keep feeding schedules working without cloud services,
- make the device maintainable over time,
- provide full hardware and firmware sources,
- make future improvements easier (battery backup, sensors, telemetry, OTA, etc.).

---

## Main features

- **ESP32-C3 Mini** based controller
- **DS3231 RTC** for local timekeeping and schedule persistence
- **Standalone AP mode web UI** for setup and manual feed triggering
- **2 automatic feeding slots** with configurable hour / minute / dose count
- **Manual local feed button**
- **Endstop-based motor cycle control**
- EEPROM-backed configuration persistence
- Fritzing-based prototyping and PCB workflow
- Gerber export ready for fabrication

---

## Repository structure

```text
croquettor/
├── README.md
├── LICENSE
├── LICENSE-HARDWARE
├── .gitignore
├── firmware/
│   └── croquettorFW.ino
├── hardware/
│   ├── fritzing/
│   │   └── README.md
│   ├── gerber/
│   │   └── README.md
│   └── pcb/
│       └── README.md
├── docs/
│   ├── architecture.md
│   ├── wiring.md
│   ├── assembly.md
│   ├── usage.md
│   ├── firmware-analysis.md
│   └── troubleshooting.md
├── images/
│   └── README.md
└── config/
    └── example_config.h
```

---

## Hardware overview

### Core components

- ESP32-C3 Mini
- DS3231 RTC (I²C)
- Motor driver or power switching stage for the feeder motor
- Endstop / position sensor input
- User button
- Status LED
- Regulated power supply matching feeder electronics

### Pin mapping from current firmware

| Signal | GPIO |
|---|---:|
| Button | 10 |
| LED | 2 |
| Endstop | 3 |
| Motor control | 4 |
| RTC interrupt | 5 |
| SDA | 8 |
| SCL | 9 |

> Note: `RTC_INT_PIN` is currently defined in the firmware but not yet used.

---

## Software overview

The firmware is written for the Arduino ecosystem on ESP32 and currently relies on:

- `WiFi.h`
- `WebServer.h`
- `DNSServer.h`
- `Wire.h`
- `RTClib.h`
- `EEPROM.h`

### Current functional behavior

- On boot, configuration is loaded from EEPROM.
- The ESP32 can expose a **Wi-Fi access point** (`croquettor`) with an embedded web UI.
- A **short button press** triggers manual feeding.
- A **long button press (~3 s)** enables the AP + web server for a limited time.
- Two independent auto-feed schedules can be stored.
- The RTC keeps time and enables local operation without internet.
- The motor runs until the endstop transitions through a full cycle.

---

## Web interface

Current UI features:

- manual feed trigger,
- RTC time display,
- RTC time adjustment,
- configuration of 2 automatic feeding schedules,
- countdown before the AP/web server stops automatically.

The AP credentials are currently hardcoded in firmware:

- SSID: `croquettor`
- Password: `nounette`

This is acceptable for prototyping, but should be changed before wider release.

---

## Build and flash

### Arduino IDE

1. Install ESP32 board support.
2. Open `firmware/croquettorFW.ino`.
3. Select the proper **ESP32-C3** board.
4. Install required libraries:
   - RTClib
5. Compile and flash.

### PlatformIO

A future PlatformIO environment can easily be added, but the current repository is Arduino-first.

---

## Usage

### Local usage

- Power on the feeder.
- Short press the button to dispense one dose.
- Long press the button for about 3 seconds to enable the configuration portal.
- Connect to the `croquettor` Wi-Fi network.
- Open the captive portal or navigate to `192.168.4.1`.
- Configure the RTC and automatic schedules.

### Automatic feeding

Two automatic schedules are available. Each schedule stores:

- enabled / disabled state,
- hour,
- minute,
- number of doses.

---

## Safety and limitations

This project controls a motorized mechanism intended to feed an animal. Use it carefully.

Important current limitations:

- no motor timeout if the endstop never changes state,
- no authentication on HTTP commands,
- hardcoded AP credentials,
- no anti-jam detection,
- no battery-backed state validation beyond RTC timekeeping,
- no formal hardware failsafe verification.

Do **not** rely on this system for unattended animal care until you have thoroughly validated your own hardware build.

---

## Project status

Current state:

- firmware prototype available,
- breadboard / prototyping board available,
- Fritzing PCB available,
- Gerber exported,
- repository structure prepared for publication.

Recommended next steps:

- add real hardware files (`.fzz`, Gerbers, board PNGs),
- add PCB photos and feeder teardown photos,
- add motor driver details and BOM,
- add a schematic-level wiring diagram,
- harden firmware before production use.

---

## Licensing

This project uses **dual licensing by artifact type**:

### Code / firmware

- Licensed under the **MIT License**
- See [`LICENSE`](LICENSE)

### Hardware design files

- Licensed under **CERN Open Hardware Licence Version 2 - Strongly Reciprocal**
- See [`LICENSE-HARDWARE`](LICENSE-HARDWARE)

### Practical scope

The following are intended to be treated as **hardware design documentation** under CERN-OHL-S:

- Fritzing design files
- schematics
- PCB layouts
- fabrication files / Gerbers
- mechanical or assembly design files directly tied to the board

The following are intended to be treated as **software** under MIT:

- Arduino / ESP32 firmware
- helper scripts
- software configuration examples

---

## Disclaimer

This repository documents an independent retrofit project.

All product names, trademarks, and brand names are the property of their respective owners and are used only to describe compatibility or the original target hardware.

---

## Contributing

Contributions are welcome, especially for:

- firmware hardening,
- AP / captive portal improvements,
- better scheduling logic,
- jam detection,
- safer motor control,
- cleaner PCB revisions,
- documentation and assembly photos.

---

## Acknowledgment

Croquettor exists to make repair, customization, and long-term ownership easier for connected pet hardware.
