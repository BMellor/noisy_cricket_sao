#include "cricket.h"
#include "audio.h"
#include "config.h"
#include "i2c_target.h"
#include "led.h"
#include "mic.h"
#include "timebase.h"
#include <stdbool.h>

// Everything from PWM_WARMUP to SETTLE runs with the mic blanked, through every chirp and gap of the group
typedef enum { IDLE, ARMED, REST, PWM_WARMUP, AMP_WARMUP, CHIRP, GAP, BEEP, SETTLE } state_t;

static state_t state;
static uint32_t t0, wait_ms;
static uint8_t chirps_left;
static uint16_t threshold;
static bool quiet;
static uint32_t rng = 0x2545F491u;
static uint8_t led_r, led_g, led_b, fade;
static bool beep_pending, after_beep;
static uint16_t beep_hz, beep_ms;

static uint32_t rand32(void) {
  rng ^= mic_entropy();
  if (rng == 0)
    rng = 0x2545F491u;
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng;
}

static uint32_t rand_range(uint32_t lo, uint32_t hi) { return lo + rand32() % (hi - lo + 1); }

static void update_quiet(void) {
  uint16_t level = mic_level_fast();
  if (quiet) {
    if (level > threshold + threshold / HYSTERESIS_DIV)
      quiet = false;
  } else if (level < threshold) {
    quiet = true;
  }
}

static void show_led(void) { led_set_rgb(led_r, fade > led_g ? fade : led_g, led_b); }

static void enter(state_t s, uint32_t now) {
  state = s;
  t0 = now;
}

static void start_chirp(uint32_t now) {
  audio_play_chirp();
  enter(CHIRP, now);
}

static void power_up(uint32_t now) {
  mic_set_blanking(true);
  audio_pwm_enable(true);
  enter(PWM_WARMUP, now);
}

static void start_group(uint32_t now) {
  chirps_left = (uint8_t)rand_range(CHIRPS_MIN, CHIRPS_MAX);
  power_up(now);
}

static void start_beep(uint32_t now) {
  beep_pending = false;
  audio_play_tone(beep_hz, beep_ms);
  enter(BEEP, now);
}

// A beep overrides chirping: it cuts in at once if the amp is already warm, otherwise powers up first
static void handle_beep_request(uint32_t now) {
  switch (state) {
  case CHIRP:
  case GAP:
  case BEEP:
    start_beep(now);
    break;
  default:
    power_up(now);
    break;
  }
}

void cricket_init(uint16_t t) { threshold = t; }

void cricket_poll(void) {
  uint32_t now = get_timestamp_ms();
  uint32_t dt = now - t0;
  uint8_t new_fade = 0;

  // During warm-up a pending beep just waits; AMP_WARMUP starts it instead of a chirp
  if (beep_pending && state != PWM_WARMUP && state != AMP_WARMUP) {
    handle_beep_request(now);
    dt = 0;
  }

  switch (state) {
  case IDLE:
    update_quiet();
    if (quiet)
      enter(ARMED, now);
    break;
  case ARMED:
    update_quiet();
    if (!quiet) {
      enter(IDLE, now);
    } else if (dt >= PREROLL_MS) {
      start_group(now);
    }
    break;
  case REST:
    update_quiet();
    if (!quiet)
      enter(IDLE, now);
    else if (dt >= wait_ms)
      start_group(now);
    break;
  case PWM_WARMUP:
    if (dt >= PWM_WARMUP_MS) {
      audio_amp_enable(true);
      enter(AMP_WARMUP, now);
    }
    break;
  case AMP_WARMUP:
    if (dt >= AMP_WARMUP_MS) {
      if (beep_pending)
        start_beep(now);
      else
        start_chirp(now);
    }
    break;
  case CHIRP:
    if (audio_busy()) {
      uint8_t p = audio_progress();
      new_fade = (uint8_t)(p < 128 ? p * 2 : (255 - p) * 2);
    } else if (--chirps_left) {
      wait_ms = rand_range(CHIRP_GAP_MIN_MS, CHIRP_GAP_MAX_MS);
      enter(GAP, now);
    } else {
      audio_amp_enable(false);
      audio_pwm_enable(false);
      enter(SETTLE, now);
    }
    break;
  case GAP:
    if (dt >= wait_ms)
      start_chirp(now);
    break;
  case BEEP:
    if (!audio_busy()) {
      audio_amp_enable(false);
      audio_pwm_enable(false);
      after_beep = true;
      enter(SETTLE, now);
    }
    break;
  case SETTLE:
    if (dt >= SETTLE_MS) {
      // quiet still holds its pre-group value (frozen while blanked); REST/IDLE re-check it
      mic_set_blanking(false);
      if (after_beep) {
        after_beep = false;
        enter(IDLE, now); // after a beep, chirping needs the full pre-roll again
      } else {
        wait_ms = rand_range(REST_MIN_MS, REST_MAX_MS);
        enter(REST, now);
      }
    }
    break;
  }

  if (new_fade != fade) {
    fade = new_fade;
    show_led();
  }
}

void cricket_set_threshold(uint16_t t) { threshold = t; }

void cricket_beep(uint16_t freq_hz, uint16_t duration_ms) {
  if (freq_hz == 0 || duration_ms == 0)
    return;
  if (freq_hz < BEEP_MIN_HZ)
    freq_hz = BEEP_MIN_HZ;
  if (freq_hz > BEEP_MAX_HZ)
    freq_hz = BEEP_MAX_HZ;
  beep_hz = freq_hz;
  beep_ms = duration_ms;
  beep_pending = true;
}

uint16_t cricket_threshold(void) { return threshold; }

void cricket_set_led(uint8_t r, uint8_t g, uint8_t b) {
  led_r = r;
  led_g = g;
  led_b = b;
  show_led();
}

uint8_t cricket_status(void) {
  uint8_t s = quiet ? STATUS_QUIET : 0;
  if (state == ARMED || state == REST)
    s |= STATUS_ARMED;
  else if (state != IDLE)
    s |= STATUS_CHIRPING;
  return s;
}
