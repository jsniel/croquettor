# Troubleshooting

## RTC not detected

- Check SDA/SCL wiring
- Check module power
- Confirm the board uses the expected I2C pins
- Confirm pull-ups if needed

## Motor does not stop

- Check endstop polarity
- Check endstop wiring continuity
- Verify mechanism actually reaches the switch
- Power off immediately if the motor stalls continuously

## AP does not appear

- Hold the button long enough
- Confirm ESP32-C3 booted correctly
- Check serial logs
- Verify the AP was not already auto-stopped

## Wrong automatic feeding time

- Confirm RTC time
- Confirm the schedule is enabled
- Confirm EEPROM settings were saved
