#pragma once

#include <stdbool.h>
#include <stdint.h>

void audio_init(void);

// PWM carrier on/off (output idles at midscale) and amplifier shutdown control
void audio_pwm_enable(bool enable);
void audio_amp_enable(bool enable);

// Play one chirp: CHIRP_REPS playbacks of the LUT, each followed by CHIRP_REP_GAP_MS of midscale
void audio_play_chirp(void);
bool audio_busy(void);

// 0..255, see DEFAULT_VOLUME
void audio_set_volume(uint8_t volume);
uint8_t audio_volume(void);

// Progress through the current chirp, 0..255 (255 when idle)
uint8_t audio_progress(void);

// Called from the mic ISR once per output sample
void audio_tick(void);
