# Firmware analysis

This document summarizes the current behavior of `croquettorFW.ino`.

## Strengths

- clear single-file prototype,
- practical AP-only setup mode,
- persistent schedule storage,
- DS3231 support,
- local scheduling independent from cloud services,
- endstop-based mechanical cycle handling.

## Main weaknesses

### 1. Motor loop can block forever

The distribution logic waits indefinitely for endstop transitions:

- wait until endstop becomes HIGH,
- then wait until it becomes LOW.

If the endstop is broken, inverted, disconnected, or if the mechanism jams, the firmware may block forever while the motor remains enabled.

### 2. Open HTTP endpoints

The AP exposes direct GET endpoints for:

- `/feed`
- `/settime`
- `/setauto`

There is no authentication, CSRF protection, or command confirmation.

### 3. Hardcoded AP credentials

SSID and password are compiled into firmware. This is acceptable for a lab prototype but weak for field use.

### 4. RTC health handling can be improved

`rtc.begin()` is called repeatedly in the main loop through LED status logic. This is unnecessary and may hide hardware-state problems behind repeated reinitialization attempts.

### 5. Automatic schedule deduplication is minute-based only

The `hasDistributedX` flags are reset when the current minute differs from the schedule minute.
This usually works, but it is not tied to date tracking. A more explicit last-trigger timestamp would be more robust.

### 6. RTC_INT_PIN is unused

This indicates room for a future interrupt-driven design using RTC alarms instead of polling.

## Recommended improvements

- add a motor timeout,
- add jam / fault state handling,
- add endpoint authentication or a setup token,
- store AP credentials in config,
- switch from EEPROM emulation to Preferences/NVS,
- track last executed schedule using date + minute,
- use DS3231 alarms on `RTC_INT_PIN`,
- split firmware into modules.
