# esp32-obd2-v2

OBDII based dashboard.

![Board](https://raw.githubusercontent.com/akos-sereg/esp32-obd2-v2/master/docs/images/20200604_125736.jpg)

## Usage

### Display

- There is a led strip (3 green, 1 yellow, 1 red) which is supposed to display the Engine Load
- LCD display shows more information, based on it's mode set by up / down buttons

### Buttons

- On/Off: led indicates if devices is switched on or off. If device is off, it is not making OBD2 calls, but still listens for key press events. In Off -> On transition, esp_restart() is called to start from scratch
- Up (top right): mode selection
- Down (bottom right): mode selection
- Toggle led-strip: switch led-strip on/off

## Configuration

- Polling interval of Engine Load (led strip): `BT_ENGINE_LOAD_POLL_INTERVAL` in `include/protocol.h`
- Restart polling Engine Load in case of OBD2 failure: `BT_RESTART_POLLING_ENGINE_LOAD_AFTER` in `include/protocol.h`
- Fuel tank in litres: `FUEL_TANK_LITER` in `include/app_main.h`
- Your OBD2 device's MAC address: `remote_device_addr_1` in `bluetooth.c`
- Your car's average fuel consumption: `AVERAGE_FUEL_CONSUMPTION_PER_100_KM` in `include/app_main.h`
- Led strip to display Engine Load or RPM: `LED_STRIP_DISPLAYS_RPM` in `include/app_main.h`
- Long key press interval (to set backlight on/off) `LONG_KEYPRESS_INTERVAL_MS` in `include/switches.h`
- In case your OBD2 device requires PIN authentication, update this line `memcpy(pin_code, "1234", 4);` in `bluetooth.c`

## How to test

Download `BT SPP Server` android app, and configure esp32 device to use your phone's MAC address when connecting via 
bluetooth. Open BT SPP Server app, and wait until it says "Connected". If esp32 device cannot connect to your server, 
open bluetooth settings (eg. list bluetooth devices in range), then navigate back to the app - this usually helps, most probably 
android starts advertising when you are at bluetooth settings screen.

### Led strip

Assuming that your device is configured to show RPM, just type `410C1234` into BT SPP Server app (eg. send it to the device). 
Device will automatically handle your command as OBD2 response (41) for RPM (0C) with value 0x12 0x34 and it will display it on the led strip.

For Engine Load, use command `41051234` (05 is Engine Load)

You might want to configure poll interval from 200 ms to 10 seconds in `include/protocol.h` for better testing

## Circuit

Connectors to esp32 based control module

[RED  ] +5 V
[BLUE ] GND
[BLACK] Led on the Button (yellow, 110 Ohm)
[BLACK] STRIP 1 (green, 110 Ohm)
[BLACK] STRIP 2 (green, 110 Ohm)
[BLACK] STRIP 3 (green, 110 Ohm)
[BLACK] STRIP 4 (yellow, 110 Ohm)
[BLACK] STRIP 5 (red, 500 Ohm)

[YELLOW] CLK for LCD
[BLACK ] DATA for LCD
[BLACK ] Button - top/right
[BLACK ] Button - bottom/right
[BLACK ] Button - on/off 
[BLACK ] Button - bottom/left

Switches: GPIOs are grounded via 1.5k Ohm resistors. Signal goes through 1k resistors.

Note: Button GPIO orders are screwed up on physical control panel (soldering fail), code follows the order in this repo, above 
docs might be mistaken, but easy to fix (by code).

## ESP-IDF

espressif/esp-idf commit hash 451f69cc2e11cf45e3a72905c9fb162ca9a08936

This project works with esp-idf at above commit hash. It did not work on a more up-to-date version however - that problem 
might have been fixed in esp-idf by now.

## Configure

Get your OBD2 bluetooth device's address (plug into your car, check what device you see on your smartphone). 
Update `main/bluetooth.c`, find `remote_device_addr_1` and set it to your OBD2 device's address. You can set a 
secondary MAC address as well: `remote_device_addr_2`, esp32 device will connect to the one that is available. 
Whenever you are testing in your car, disable your bluetooth to make sure that device will connect to the real OBD2 device, 
not to your phone.

## Build

Options choose step:
    1. idf.py menuconfig.
    2. enter menuconfig "Component config", choose "Bluetooth"
    3. enter menu Bluetooth, choose "Classic Bluetooth" and "SPP Profile"
    4. choose your options.
    
## Debug

Not sure why, but `make monitor` works only if `led_strip_init()` is not called in `app_main.c`

## TODOs

- Button keypress sometimes not recognized: use a capacitor in control panel to make sure that keypress survives CPU interrupts (or increase CPU frequency if possible)