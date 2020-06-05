#include "include/state.h"

app_state_t app_state;

void reset_app_state() {
    app_state.obd2_values.distance_to_empty_km = -1;
    app_state.obd2_values.engine_load = 0;
    app_state.obd2_values.coolant_temp_in_celsius = -1;
    app_state.obd2_values.intake_air_temp_in_celsius = -1;
    app_state.obd2_values.ambient_air_temp_in_celsius = -1;
    app_state.obd2_values.battery_voltage = -1.0;
    app_state.obd2_values.rpm = 0;
    app_state.obd2_values.fuel_level = 0;
    app_state.obd2_values.fuel_in_liter = -1.0;
    app_state.obd2_values.altitude_in_meters = -1;
    app_state.obd2_values.abs_barometric_pressure = -1;
}

// below values are being used when device connects to phone, instead of the real
// obd2 device in the car
void reset_app_state_demo() {
    app_state.obd2_values.distance_to_empty_km = 243;
    app_state.obd2_values.engine_load = 3;
    app_state.obd2_values.coolant_temp_in_celsius = 87;
    app_state.obd2_values.intake_air_temp_in_celsius = 45;
    app_state.obd2_values.ambient_air_temp_in_celsius = 23;
    app_state.obd2_values.battery_voltage = 12.2;
    app_state.obd2_values.rpm = 2800;
    app_state.obd2_values.fuel_level = 56;
    app_state.obd2_values.fuel_in_liter = 43.0;
    app_state.obd2_values.altitude_in_meters = 232;
    app_state.obd2_values.abs_barometric_pressure = 912;
}