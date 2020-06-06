#include "include/app_main.h"
#include <inttypes.h>

void main_task(void * pvParameter)
{
    int cnt = 0;
    int tick_rate_ms = 50;
    int tick_rate_on_phone_ms = 500;
    int request_sent_in_iteration = 0;
    int64_t now;

    // initializing app state
    // see include/state.h fore more details about state fields
    app_state.obd2_bluetooth.displaying_connected = 0;
    app_state.obd2_bluetooth.displaying_connected_elapsed_ms = 0;
    app_state.obd2_bluetooth.displaying_connecting_elapsed_ms = 0;
    app_state.obd2_bluetooth.displayed_connected = 0;
    app_state.obd2_bluetooth.is_connected = 0;

    // initializing LCD, Bluetooth, Input switch
    led_strip_init();
    setup_switches();
    i2c_master_init();
    led_strip_power_on_refresh();

    led_strip_set(0);
    if (!app_state.device_on) {
        lcd_display_text("Going to sleep", "");
        led_strip_animation();
        lcd_turn_off();
    }
    else {
        // bootup displays
        lcd_display_text("Connecting to", "OBD2 device");
        led_strip_animation();
    }

    while(1) {
        cnt++;
        request_sent_in_iteration = 0;

        // led_strip_set(cnt % 6);

        if (app_state.device_on) {

            // measure time spent on displaying "Connecting to bluetooth" and "Connected" LCD messages
            if (cnt == 10) {
                cnt = 0;

                if (app_state.obd2_bluetooth.displaying_connected || !app_state.obd2_bluetooth.is_connected) {
                    vTaskDelay(500 / portTICK_RATE_MS);
                }

                if (app_state.obd2_bluetooth.displaying_connected) {
                    app_state.obd2_bluetooth.displaying_connected_elapsed_ms += 500;
                }

                if (!app_state.obd2_bluetooth.is_connected) {
                    app_state.obd2_bluetooth.displaying_connecting_elapsed_ms += 500;
                }
            }

            // connected to bluetooth, notify user on display for a couple of seconds
            if (app_state.obd2_bluetooth.is_connected
            && !app_state.obd2_bluetooth.displaying_connected
            && !app_state.obd2_bluetooth.displayed_connected) {
                lcd_display_text("Connected.", "");
                led_strip_set(0);
                app_state.obd2_bluetooth.displaying_connected = 1;
                app_state.obd2_bluetooth.displayed_connected = 1;
            }

            // connected to bluetooth, poll data periodically - responses will be handled automatically, we don't have to worry about
            // processing them and triggering the new requests

            if (app_state.obd2_bluetooth.is_connected) {
                now = get_epoch_milliseconds();

                // sending request - value for LCD page
                if ((get_time_last_lcd_data_sent() + BT_LCD_DATA_POLLING_INTERVAL) < now) {
                    bt_send_data(get_lcd_page_obd_code()); // OBD PID of current page displayed by LCD
                    reset_time_last_lcd_data_sent();
                    request_sent_in_iteration = 1;
                }

                // sending request - realtime RPM or Engine Load - keep polling even if last time we failed to process response
                if (app_state.led_strip_on && !request_sent_in_iteration) {
                    if ((bt_get_last_request_sent() + BT_ENGINE_LOAD_POLL_INTERVAL) < now) {
                        bt_send_data(LED_STRIP_DISPLAYS_RPM ? obd2_request_engine_rpm() : obd2_request_calculated_engine_load());
                    }
                }
            }

            // connected to bluetooth OBD2 already, displaying data - one time refresh LCD
            if (app_state.obd2_bluetooth.displaying_connected) {
                app_state.obd2_bluetooth.displaying_connected_elapsed_ms += tick_rate_ms;
                if (app_state.obd2_bluetooth.displaying_connected_elapsed_ms > 3000) {
                    app_state.obd2_bluetooth.displaying_connected = 0;
                    refresh_lcd_display();
                }
            }

            // connecting to bluetooth
            if (!app_state.obd2_bluetooth.is_connected) {
                app_state.obd2_bluetooth.displaying_connecting_elapsed_ms += tick_rate_ms;
            } else {
                app_state.obd2_bluetooth.displaying_connecting_elapsed_ms = 0;
            }

            if (app_state.obd2_bluetooth.displaying_connecting_elapsed_ms > 15000) {
                // could not connect to bluetooth, or connection is lost for 15 seconds
                // leaving a message on LCD display and rebooting esp32 device (restart tro reconnect)
                lcd_display_text("Restarting ...", "");
                nvs_shutdown();
                esp_restart();
            }
        }

        vTaskDelay((app_state.obd2_bluetooth.is_connected_to_phone ? tick_rate_on_phone_ms : tick_rate_ms) / portTICK_RATE_MS);
    }
}

void app_main()
{
    init_bluetooth();
    init_nvs_store();

    // load LCD display mode (page) from NVS memory - refresh_lcd_display() will show
    // the correct page when it is called
    LCD_DISPLAY_MODE = get_nvs_value(NVS_KEY_MODE);
        // first time since esp32 deploy, set to default
    if (LCD_DISPLAY_MODE == -1) {
        // first time since esp32 deploy, set to default
        LCD_DISPLAY_MODE = 0;
        set_nvs_value(NVS_KEY_MODE, LCD_DISPLAY_MODE);
    }

    reset_app_state();
    app_state.device_on = get_nvs_value(NVS_KEY_IS_ON);
    if (app_state.device_on == -1) {
        // first time since esp32 deploy, set to default
        app_state.device_on = 1;
        set_nvs_value(NVS_KEY_IS_ON, app_state.device_on);
    }

    app_state.led_strip_on = get_nvs_value(NVS_KEY_IS_LS_ON);
    if (app_state.led_strip_on == -1) {
        // first time since esp32 deploy, set to default
        app_state.led_strip_on = 1;
        set_nvs_value(NVS_KEY_IS_LS_ON, app_state.led_strip_on);
    }

    xTaskCreate(&main_task, "main_task", 4096, NULL, 5, NULL);
}

