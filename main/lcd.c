#include "include/lcd.h"

bool lcd_backlight = true;
volatile int lcd_is_locked = 0;
long lcd_locked_timestamp_ms = 0;
int lcd_request_count = 0;
char previous_title_line[32];
char previous_data_line[32];

int lcd_is_resource_locked();
void lcd_lock();
void lcd_unlock();

/*
   Adding / modifying LCD display pages:
   - refresh_lcd_display() to display the text and value
   - get_lcd_page_obd_code() to respond the correct OBD PID
   - command-handler.c - handle_obd2_response() to calculate value
   - switches.c - MAX_LCD_DISPLAY_MODE to be set
*/
void i2c_master_init(void)
{
    // Set up I2C
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_LEN,
                       I2C_MASTER_TX_BUF_LEN, 0);
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    smbus_init(smbus_info, i2c_num, address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    // Set up the LCD1602 device
    lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true);

    // turn on backlight
    int32_t backlight_on = get_nvs_value(NVS_KEY_LCD_BACKLIGHT);
    lcd_backlight = (backlight_on == -1 || backlight_on == 1) ? true : false;
    i2c_lcd1602_set_backlight(lcd_info, lcd_backlight);
}

void lcd_display_text(char *line1, char *line2) {

    if (strlen(line1) > 16 || strlen(line2) > 16) {
        return;
    }

    if (lcd_is_resource_locked()) {
        // if we are here, and we initiate i2c communication with LCD display, we might cause
        // an interference with another thread's i2c communication, and LCD display would show
        // screwed up characters
        return;
    }

    lcd_lock();

    i2c_lcd1602_clear(lcd_info);
    i2c_lcd1602_write_string(lcd_info, line1);

    if (line2 != NULL) {
        i2c_lcd1602_move_cursor(lcd_info, 0, 1);
        i2c_lcd1602_write_string(lcd_info, line2);
    }

    lcd_unlock();
}

void toggle_lcd_backlight() {
    if (lcd_is_resource_locked()) {
        // if we are here, and we initiate i2c communication with LCD display, we might cause
        // an interference with another thread's i2c communication, and LCD display would show
        // screwed up characters
        return;
    }

    lcd_lock();

    lcd_backlight = !lcd_backlight;
    i2c_lcd1602_set_backlight(lcd_info, lcd_backlight);
    set_nvs_value(NVS_KEY_LCD_BACKLIGHT, lcd_backlight ? 1 : 0);

    lcd_unlock();
}

void lcd_turn_off() {
    if (lcd_is_resource_locked()) {
        // if we are here, and we initiate i2c communication with LCD display, we might cause
        // an interference with another thread's i2c communication, and LCD display would show
        // screwed up characters
        return;
    }

    lcd_lock();

    i2c_lcd1602_clear(lcd_info);
    i2c_lcd1602_set_backlight(lcd_info, false);

    lcd_unlock();
}

void refresh_lcd_display() {
    char line[32], title_line[32];

    if (LCD_DISPLAY_MODE < 0) {
        LCD_DISPLAY_MODE = 0;
    }

    if (LCD_DISPLAY_MODE > MAX_LCD_DISPLAY_MODE) {
        LCD_DISPLAY_MODE = MAX_LCD_DISPLAY_MODE;
    }

    switch (LCD_DISPLAY_MODE) {
        case 0:
            if (app_state.obd2_values.fuel_in_liter == -1.0) {
                sprintf(title_line, "Fuel");
                sprintf(line, "(no data)");
            } else {
                sprintf(title_line, "Fuel: %.1f l", app_state.obd2_values.fuel_in_liter);
                sprintf(line, "%d km to empty", app_state.obd2_values.distance_to_empty_km);
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        case 1:
            sprintf(title_line, "Engine Coolant");
            if (app_state.obd2_values.coolant_temp_in_celsius == -1) {
                sprintf(line, "(no data)");
            }
            else {
                sprintf(line, "%d %cC", app_state.obd2_values.coolant_temp_in_celsius, 223);
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        case 2:
            // based on http://popupbackpacker.com/wp-content/uploads/2013/12/State-of-Charge-Chart-Typical-Internet.jpg
            // based on https://www.hestore.hu/prod_10033247.html
            // -> x > 13.9        -> Green
            // -> 11.8 < x < 13.8 -> Orange
            // -> x < 11.8        -> Red

            sprintf(title_line, "Battery");
            if (app_state.obd2_values.battery_voltage == -1.0) {
                sprintf(line, "(no data)");
            } else {
                if (app_state.obd2_values.battery_voltage > 12.6) {
                    sprintf(line, "%.1f V (100%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 12.5) {
                    sprintf(line, "%.1f V (90%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 12.42) {
                    sprintf(line, "%.1f V (80%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 12.32) {
                    sprintf(line, "%.1f V (70%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 12.20) {
                    sprintf(line, "%.1f V (60%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 12.06) {
                    sprintf(line, "%.1f V (50%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 11.9) {
                    sprintf(line, "%.1f V (40%%)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 11.75) {
                    sprintf(line, "%.1f V (30%% LOW)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 11.58) {
                    sprintf(line, "%.1f V (20%% LOW)", app_state.obd2_values.battery_voltage);
                } else if (app_state.obd2_values.battery_voltage > 11.31) {
                    sprintf(line, "%.1f V (10%% LOW)", app_state.obd2_values.battery_voltage);
                } else {
                    sprintf(line, "%.1f V (0%% LOW)", app_state.obd2_values.battery_voltage);
                }
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        case 3:
            sprintf(title_line, "Intake air temp.");
            if (app_state.obd2_values.intake_air_temp_in_celsius == -1) {
                sprintf(line, "(no data)");
            }
            else {
                sprintf(line, "%d %cC", app_state.obd2_values.intake_air_temp_in_celsius, 223);
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        case 4:
            sprintf(title_line, "Outside temp.");
            if (app_state.obd2_values.ambient_air_temp_in_celsius == -1) {
                sprintf(line, "(no data)");
            }
            else {
                sprintf(line, "%d %cC", app_state.obd2_values.ambient_air_temp_in_celsius, 223);
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        case 5:
            sprintf(title_line, "Altitude");
            if (app_state.obd2_values.altitude_in_meters == -1) {
                if (app_state.obd2_values.abs_barometric_pressure == -1 && app_state.obd2_values.ambient_air_temp_in_celsius == -1) {
                    sprintf(line, "(no data)");
                }
                else {
                    sprintf(line, "(1/2 data)");
                }

            }
            else {
                sprintf(line, "%d m (+-30m)", app_state.obd2_values.altitude_in_meters);
            }

            if (strcmp(previous_data_line, line) == 0 && strcmp(previous_title_line, title_line) == 0) {
                // we want to display the same value, ignore updating LCD, as LCD updates are always visible (eg. flickering)
                return;
            }

            lcd_display_text(title_line, line);
            break;

        default:
            return;
    }

    sprintf(previous_title_line, "%s", title_line);
    sprintf(previous_data_line, "%s", line);
}

char *get_lcd_page_obd_code() {
    lcd_request_count++;

    switch(LCD_DISPLAY_MODE) {
        case 0:
            // fetching fuel level, this is required to calculate "Distance to empty"
            return obd2_request_fuel_level();

        case 1:
            return obd2_request_engine_coolant_temp();

        case 2:
            return obd2_request_battery_voltage();

        case 3:
            return obd2_request_intake_air_temperature();

        case 4:
            return obd2_request_ambient_air_temperature();

        case 5:
            // ambient air temperature and barometric pressure are both needed for altitude calculation.
            // so sometimes we fetch the barometric pressure, and sometimes we fetch the air temperature. once both values
            // are fetched, altitude can be calculated and it can be displayed on the LCD. this tweak means that the
            // value will be visible only after 2 iterations of LCD data polling, but thats fine.
            return lcd_request_count % 2 == 0 ?
                obd2_request_abs_barometric_pressure() : obd2_request_ambient_air_temperature();

        default:
            return obd2_request_fuel_level();
    }

    return NULL;
}

int lcd_is_resource_locked() {
    int current_time = get_epoch_milliseconds();

    // resource is not locked
    if (!lcd_is_locked) {
        return 0;
    }

    // check if timeout reached and unlock lcd resource
    if ((current_time - lcd_locked_timestamp_ms) > LCD_LOCK_TIMEOUT) {
        lcd_is_locked = 0;
        return 0;
    }

    // resource is locked and cannot be used
    return 1;
}

void lcd_lock() {
    lcd_is_locked = 1;
    lcd_locked_timestamp_ms = get_epoch_milliseconds();
}

void lcd_unlock() {
    lcd_is_locked = 0;
}