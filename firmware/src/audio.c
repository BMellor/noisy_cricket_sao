#include "audio.h"
#include "chirp_lut.h"
#include "config.h"
#include "pins.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timera.h>

#define PWM_MID 128
#define REP_GAP_SAMPLES ((uint16_t)(CHIRP_REP_GAP_MS * SAMPLE_RATE_HZ / 1000))
#define REP_SAMPLES (CHIRP_LEN + REP_GAP_SAMPLES)
#define CHIRP_SAMPLES ((uint32_t)REP_SAMPLES * CHIRP_REPS)

static volatile uint16_t pos;     // within the current playback + gap
static volatile uint16_t elapsed; // within the whole chirp
static volatile uint8_t reps_left;
static volatile uint8_t volume = DEFAULT_VOLUME;
static volatile uint16_t gain = DEFAULT_VOLUME + (DEFAULT_VOLUME >> 7); // Q8

static inline void set_pwm(uint8_t v) { TIMA0->COUNTERREGS.CC_01[0] = v; }

static inline uint8_t scaled(uint8_t v) { return (uint8_t)(PWM_MID + ((((int32_t)v - PWM_MID) * gain) >> 8)); }

void audio_init(void) {
  DL_TimerA_reset(TIMA0);
  DL_TimerA_enablePower(TIMA0);

  // Amp shutdown is active high
  DL_GPIO_initDigitalOutput(AMP_SD.iomux);
  DL_GPIO_enableOutput(AMP_SD.port, AMP_SD.pinmask);
  DL_GPIO_setPins(AMP_SD.port, AMP_SD.pinmask);

  DL_GPIO_initPeripheralOutputFunction(AUDIO.iomux, IOMUX_PINCM29_PF_TIMA0_CCP0);

  // 24 MHz / 256 = 93.75 kHz carrier, one PWM period per SPI word. The sample is updated from the mic ISR
  // every 4 periods; the zero-event shadow load keeps ISR jitter off the output edge.
  DL_TimerA_ClockConfig clkCfg = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
      .prescale = 0U,
  };
  DL_TimerA_setClockConfig(TIMA0, &clkCfg);

  DL_TimerA_PWMConfig pwmCfg = {
      .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
      .period = 255,
      .isTimerWithFourCC = true,
      .startTimer = DL_TIMER_STOP,
  };
  DL_TimerA_initPWMMode(TIMA0, &pwmCfg);

  DL_TimerA_setCCPDirection(TIMA0, DL_TIMER_CC0_OUTPUT);
  DL_TimerA_setCaptureCompareValue(TIMA0, PWM_MID, DL_TIMER_CC_0_INDEX);
  DL_TimerA_setCaptCompUpdateMethod(TIMA0, DL_TIMER_CC_UPDATE_METHOD_ZERO_EVT, DL_TIMER_CC_0_INDEX);

  DL_TimerA_enableClock(TIMA0);
  DL_TimerA_enableShadowFeatures(TIMA0);
}

void audio_pwm_enable(bool enable) {
  if (enable) {
    set_pwm(PWM_MID);
    DL_TimerA_startCounter(TIMA0);
  } else {
    DL_TimerA_stopCounter(TIMA0);
  }
}

void audio_amp_enable(bool enable) {
  if (enable)
    DL_GPIO_clearPins(AMP_SD.port, AMP_SD.pinmask);
  else
    DL_GPIO_setPins(AMP_SD.port, AMP_SD.pinmask);
}

void audio_play_chirp(void) {
  pos = 0;
  elapsed = 0;
  reps_left = CHIRP_REPS; // written last: arms the ISR
}

bool audio_busy(void) { return reps_left != 0; }

void audio_set_volume(uint8_t v) {
  volume = v;
  gain = (uint16_t)(v + (v >> 7));
}

uint8_t audio_volume(void) { return volume; }

uint8_t audio_progress(void) {
  if (!reps_left)
    return 255;
  return (uint8_t)((uint32_t)elapsed * 255 / CHIRP_SAMPLES);
}

void audio_tick(void) {
  if (!reps_left)
    return;
  uint16_t p = pos;
  if (p < CHIRP_LEN)
    set_pwm(scaled(chirp_lut[p]));
  else if (p == CHIRP_LEN)
    set_pwm(PWM_MID);
  if (++p >= REP_SAMPLES) {
    p = 0;
    reps_left--;
  }
  pos = p;
  elapsed++;
}
