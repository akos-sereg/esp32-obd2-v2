#include "include/command-handler.h"

/**
 * fuel readings can vary depending on car's horizontal position (eg. uphill or flat ground), so
 * we always take the min fuel reading when calculating "distance to empty"
 */
double min_fuel_reading = -1.0;

void calculate_altitude(int barometric_pressure_in_kPa, int ambient_air_temp_in_celsius);

/**
 * Calculations based on: https://en.wikipedia.org/wiki/OBD-II_PIDs
 *
 * Whitespaces are removed from payload!
 * eg. "NO DATA" is now "NODATA"
 * and "41 04 3E" is now "41043E"
 */
void handle_obd2_response(char *obd2_response) {
    // sample responses:
    //
    // Request  > 01 04                - Engine Load
    // Response > 41 04 3E             - or ...
    // Response > 41043E
    //
    // Request  > 01 2F                - Fuel Level
    // Response > 41 2F DE             - or ...
    // Response > 412FDE

    int a = -1; // first byte of response
    int b = -1; // second byte of response
    int c = -1; // 3rd
    int d = -1; // 4th
    char *ptr;
    char hex_buf[3];

    if (strlen(obd2_response) >= 6
        && obd2_response[0] == 'N'
        && obd2_response[1] == 'O'
        && obd2_response[2] == 'D'
        && obd2_response[3] == 'A'
        && obd2_response[4] == 'T'
        && obd2_response[5] == 'A') {

        // NO DATA response, moving forward ...
        printf("  --> [OBD Response] NO DATA, moving forward");
        return;
    }

    // in case response is "410C1234", a = 12 in hex
    if (strlen(obd2_response) >= 6
        && ((obd2_response[4] >= '0' && obd2_response[4] <= '9') || (obd2_response[4] >= 'A' && obd2_response[4] <= 'F'))
        && ((obd2_response[5] >= '0' && obd2_response[5] <= '9') || (obd2_response[5] >= 'A' && obd2_response[5] <= 'F'))) {
        sprintf(hex_buf, "%c%c", obd2_response[4], obd2_response[5]);
        a = strtol(hex_buf, &ptr, 16);
    }

    // in case response is "410C1234", b = 34 in hex
    if (strlen(obd2_response) >= 8
        && ((obd2_response[6] >= '0' && obd2_response[6] <= '9') || (obd2_response[6] >= 'A' && obd2_response[6] <= 'F'))
        && ((obd2_response[7] >= '0' && obd2_response[7] <= '9') || (obd2_response[7] >= 'A' && obd2_response[7] <= 'F'))) {
        sprintf(hex_buf, "%c%c", obd2_response[6], obd2_response[7]);
        b = strtol(hex_buf, &ptr, 16);
    }

    // in case response is "410C12345678", c = 56 in hex
    if (strlen(obd2_response) >= 10
        && ((obd2_response[8] >= '0' && obd2_response[8] <= '9') || (obd2_response[8] >= 'A' && obd2_response[8] <= 'F'))
        && ((obd2_response[9] >= '0' && obd2_response[9] <= '9') || (obd2_response[9] >= 'A' && obd2_response[9] <= 'F'))) {
        sprintf(hex_buf, "%c%c", obd2_response[8], obd2_response[9]);
        c = strtol(hex_buf, &ptr, 16);
    }

    // in case response is "410C12345678", d = 78 in hex
    if (strlen(obd2_response) >= 12
        && ((obd2_response[10] >= '0' && obd2_response[10] <= '9') || (obd2_response[10] >= 'A' && obd2_response[10] <= 'F'))
        && ((obd2_response[11] >= '0' && obd2_response[11] <= '9') || (obd2_response[11] >= 'A' && obd2_response[11] <= 'F'))) {
        sprintf(hex_buf, "%c%c", obd2_response[10], obd2_response[11]);
        d = strtol(hex_buf, &ptr, 16);
    }

    // printf("  --> [OBD Response] values are: a = %d, b = %d\n", a, b);
    char req_test[16];
    char req_pattern[16];

    // figuring out what could have been the request
    // eg. if OBD2 response is "4104" then the request was "0104", req_test will be compared against dimensions
    sprintf(req_test, "%s", obd2_response);
    if (strlen(req_test) >= 2) {
        req_test[0] = '0';
        req_test[1] = '1';
    }

    if (!app_state.device_on) {
        // OBD response received after switching device OFF, turning led strip off
        led_strip_set(0);
        return;
    }

    // Engine Load
    sprintf(req_pattern, "%s", obd2_request_calculated_engine_load());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.engine_load = ceil(a / 2.55); // result is a number between 0 to 100 (engine load in %)

        // result is a number between 0 and 5 (can be displayed on led strip)
        // 20.0 = 100 / 5
        app_state.obd2_values.engine_load = ceil(app_state.obd2_values.engine_load / 20.0);

        if (app_state.obd2_values.engine_load > 5) {
            app_state.obd2_values.engine_load = 5;
        }

        led_strip_set(app_state.obd2_values.engine_load);
    }

    // RPM
    sprintf(req_pattern, "%s", obd2_request_engine_rpm());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        int baseline_rpm = 900;
        int max_rpm = 3600;
        int num_of_leds = 5;
        double magic = num_of_leds / (double)(max_rpm - baseline_rpm);

        app_state.obd2_values.rpm = ((256 * a) + b) / 4; // value from 0 to 16383
        app_state.obd2_values.rpm -= baseline_rpm;
        app_state.obd2_values.rpm = ceil((double)(app_state.obd2_values.rpm * (double)magic));

        if (app_state.obd2_values.rpm > num_of_leds) {
            app_state.obd2_values.rpm = num_of_leds;
        }
        // printf("Setting led strip status to: %d as a=%d, b=%d\n", app_state.obd2_values.rpm, a, b);
        led_strip_set(app_state.obd2_values.rpm);
    } // else printf("NOT Detected as RPM value\n");

    // Distance to Empty
    sprintf(req_pattern, "%s", obd2_request_fuel_level());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.fuel_level = (double)(a / (double)2.55); // fuel level in % (value from 0 to 100)
        if (min_fuel_reading > 0 && min_fuel_reading < app_state.obd2_values.fuel_level) {
            app_state.obd2_values.fuel_level = min_fuel_reading;
        } else {
            min_fuel_reading = app_state.obd2_values.fuel_level;
        }
        app_state.obd2_values.fuel_in_liter = (double)(app_state.obd2_values.fuel_level / 100) * FUEL_TANK_LITER;
        app_state.obd2_values.distance_to_empty_km = ((double)app_state.obd2_values.fuel_in_liter / (double)AVERAGE_FUEL_CONSUMPTION_PER_100_KM) * 100;
        refresh_lcd_display();
    }

    // Coolant temp
    sprintf(req_pattern, "%s", obd2_request_engine_coolant_temp());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.coolant_temp_in_celsius = a - 40;
        refresh_lcd_display();
    }

    // Intake air temperature
    sprintf(req_pattern, "%s", obd2_request_intake_air_temperature());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.intake_air_temp_in_celsius = a - 40;
        refresh_lcd_display();
    }

    // Ambient air temperature
    sprintf(req_pattern, "%s", obd2_request_ambient_air_temperature());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.ambient_air_temp_in_celsius = a - 40;

        if (app_state.obd2_values.abs_barometric_pressure != -1) {
            calculate_altitude(app_state.obd2_values.abs_barometric_pressure, app_state.obd2_values.ambient_air_temp_in_celsius);
        }

        refresh_lcd_display();
    }

    // Battery Voltage
    sprintf(req_pattern, "%s", obd2_request_battery_voltage());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.battery_voltage = (double)((double)(255 * a) + b) / (double)1000;
        refresh_lcd_display();
    }

    // Odometer
    sprintf(req_pattern, "%s", obd2_request_odometer());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        long odometer = ((a * pow(2, 24)) + (b * pow(2, 16)) + (c * pow(2, 8)) + d);

        if (app_state.start_odometer == -1) {
            // first reading, after app init
            app_state.start_odometer = odometer;
        }
        else {
            // lcd screen readings
            app_state.latest_odometer = odometer;
            refresh_lcd_display();
        }
    }

    // Barometric pressure
    sprintf(req_pattern, "%s", obd2_request_abs_barometric_pressure());
    remove_char(req_pattern, ' ');

    if (strncmp(req_test, req_pattern, 4) == 0) {
        app_state.obd2_values.abs_barometric_pressure = a;

        if (app_state.obd2_values.ambient_air_temp_in_celsius != -1) {
            calculate_altitude(app_state.obd2_values.abs_barometric_pressure, app_state.obd2_values.ambient_air_temp_in_celsius);
        }

        refresh_lcd_display();
    }
}

void calculate_altitude(int barometric_pressure_in_kPa, int ambient_air_temp_in_celsius) {
    // calculate altitude, based on this formula:
    // https://keisan.casio.com/exec/system/1224585971
    int barometric_pressure_in_hPa = barometric_pressure_in_kPa * 10;

    // barometric pressure is in kPa, possible values in hPa are in 10 hPa steps, eg. 980 -> 990 -> 1000
    // values read from OBD2 device are in format ceil(kPa), so we add 5 hPa and we will display +- 30m
    // unfortunately we cannot read the value in better precision - kPa value in int is good enough for
    // the car to operate
    barometric_pressure_in_hPa += 5;

    app_state.obd2_values.altitude_in_meters = ((pow((1013.25 / barometric_pressure_in_hPa), (double)(1/5.257)) - 1) * (ambient_air_temp_in_celsius+273.15)) / 0.0065;

}
