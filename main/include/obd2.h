#ifndef __elm_327_h_included__
#define __elm_327_h_included__

#include "bluetooth.h"

extern char *obd2_request_calculated_engine_load();
extern char *obd2_request_engine_rpm();
extern char *obd2_request_engine_coolant_temp();
extern char *obd2_request_fuel_level();
extern char *obd2_request_battery_voltage();
extern char *obd2_request_intake_air_temperature();
extern char *obd2_request_ambient_air_temperature();
extern char *obd2_request_abs_barometric_pressure();

extern void obd2_init_communication();

#endif