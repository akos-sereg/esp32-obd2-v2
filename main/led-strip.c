#include "include/led-strip.h"

void engine_load_init() {

    gpio_set_direction(LED_STRIP_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_STRIP_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_STRIP_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_STRIP_4, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_STRIP_5);
    gpio_set_direction(LED_STRIP_5, GPIO_MODE_OUTPUT);

    gpio_set_direction(LED_BUTTON, GPIO_MODE_OUTPUT);

}

/* value from 0 to 5 */

void led_strip_set(int value) {

    if (value < 0 || value > 5) {
        return;
    }

    gpio_set_level(LED_STRIP_1, value > 0 ? 1 : 0);
    gpio_set_level(LED_STRIP_2, value > 1 ? 1 : 0);
    gpio_set_level(LED_STRIP_3, value > 2 ? 1 : 0);
    gpio_set_level(LED_STRIP_4, value > 3 ? 1 : 0);
    gpio_set_level(LED_STRIP_5, value > 4 ? 1 : 0);
}

void led_strip_power_on_refresh() {
    gpio_set_level(LED_BUTTON, app_state.device_on);
}
/*
void init_animation() {
    int i = 0;
    int j = 0;

    for (j=0; j!=3; j++) {
        for (i=0; i!=10; i++) {
            led_strip_set(i);
            vTaskDelay(50 / portTICK_RATE_MS);
        }

        for (i=9; i!=-1; i--) {
            led_strip_set(i);
            vTaskDelay(50 / portTICK_RATE_MS);
        }
    }

    led_strip_set(0);
}*/