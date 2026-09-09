#pragma once

#include <stdbool.h>
#include <stdint.h>

void led_init(void);

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

void led_set_red(uint8_t value);
void led_set_green(uint8_t value);
void led_set_blue(uint8_t value);