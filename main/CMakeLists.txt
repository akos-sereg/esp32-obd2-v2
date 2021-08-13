idf_component_register(SRCS "app_main.c" "bluetooth.c" "bt-common.c" "command-handler.c" "lcd.c" "led-strip.c" "nvs-store.c" "obd2.c" "protocol.c" "state.c" "string-helper.c" "switches.c"
    "../components/esp32-i2c-lcd1602/i2c-lcd1602.c"
    "../components/esp32-smbus/smbus.c"
                    INCLUDE_DIRS ".")
