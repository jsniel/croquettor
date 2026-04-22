# Wiring

## Current firmware pinout

| Signal | GPIO | Direction | Notes |
|---|---:|---|---|
| BUTTON_PIN | 10 | Input | Internal pull-up enabled |
| LED_PIN | 2 | Output | Status LED |
| ENDSTOP_PIN | 3 | Input | Internal pull-up enabled |
| MOTOR_PIN | 4 | Output | Drives the motor control stage |
| RTC_INT_PIN | 5 | Input | Reserved / currently unused |
| SDAPIN | 8 | I2C | RTC SDA |
| SCLPIN | 9 | I2C | RTC SCL |

## Notes

- The firmware assumes the button and endstop are active-low.
- The motor output should not drive the motor directly from the MCU pin.
- Use an appropriate transistor / MOSFET / driver stage depending on motor current.
- Add flyback protection if the motor is DC and inductive.

## To document here later

- exact motor stage schematic,
- voltage rails,
- connector mapping to the original feeder,
- board revision notes.
