#include "selftest.h"
#include "audio.h"
#include "config.h"
#include "led.h"
#include "mic.h"
#include "timebase.h"
#include <ti/devices/msp/msp.h>

#define SELFTEST_WHITE_MS 500
#define SELFTEST_MIC_MS 2000
#define SELFTEST_MIC_UPDATE_MS 20

// 8 * log2(v), 3 fractional bits from the bits below the MSB
static uint8_t log2q3(uint16_t v) {
  if (v == 0)
    return 0;
  uint8_t e = 15;
  while (!(v & 0x8000)) {
    v <<= 1;
    e--;
  }
  return (uint8_t)((e << 3) | ((v >> 12) & 7));
}

// Log scale from 1 CIC LSB (level 16) to full scale (8192): blue, cyan, green, yellow, red
static void show_level(uint16_t level) {
  int32_t h = ((int32_t)log2q3(level) - 32) * 255 / 72;
  if (h < 0)
    h = 0;
  if (h > 255)
    h = 255;
  uint8_t t = (uint8_t)((h & 63) * 4);
  switch (h >> 6) {
  case 0:
    led_set_rgb(0, t, 255);
    break;
  case 1:
    led_set_rgb(0, 255, 255 - t);
    break;
  case 2:
    led_set_rgb(t, 255, 0);
    break;
  default:
    led_set_rgb(255, 255 - t, 0);
    break;
  }
}

void selftest_run(void) {
  mic_set_blanking(true);
  audio_pwm_enable(true);
  delay_ms(PWM_WARMUP_MS);
  audio_amp_enable(true);
  delay_ms(AMP_WARMUP_MS);
  for (uint8_t idx = 0; idx < 2; idx++) {
    audio_play_chirp();
    while (audio_busy()) {
      uint8_t p = audio_progress();
      led_set_green((uint8_t)(p < 128 ? p * 2 : (255 - p) * 2));
      __WFI();
    }
    led_set_rgb(0, 0, 0);
    delay_ms(CHIRP_GAP_MIN_MS);
  }
  audio_amp_enable(false);
  audio_pwm_enable(false);
  delay_ms(SETTLE_MS);
  mic_set_blanking(false);

  led_set_rgb(255, 255, 255);
  delay_ms(SELFTEST_WHITE_MS);

  for (uint32_t t = 0; t < SELFTEST_MIC_MS; t += SELFTEST_MIC_UPDATE_MS) {
    show_level(mic_level_fast());
    delay_ms(SELFTEST_MIC_UPDATE_MS);
  }
  led_set_rgb(0, 0, 0);
}
