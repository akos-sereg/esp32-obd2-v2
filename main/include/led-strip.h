#ifndef __engine_load_leds__
#define __engine_load_leds__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "switches.h"

#define LED_STRIP_1	33  // normal
#define LED_STRIP_2	25  // normal
#define LED_STRIP_3	26  // normal
#define LED_STRIP_4	23  // normal
#define LED_STRIP_5	1   // pad select
#define LED_BUTTON	27  // normal

/* when GPIO 22 is wired, button should be pressed on esp32 device when deploying */

extern void led_strip_init();

/**
 * load value can be: 0-5
 */
extern void led_strip_set(int load);

extern void led_strip_power_on_refresh();

/**
 * Plays animation when device is initialized
 */
extern void led_strip_animation();

#endif