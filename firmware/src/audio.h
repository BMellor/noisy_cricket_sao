#pragma once

#include <stdbool.h>
#include <stdint.h>

void audio_init(void);

void audio_enable(bool enable);

void chirp(void);