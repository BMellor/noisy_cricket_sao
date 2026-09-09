#include "audio.h"
#include "pins.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timera.h>

volatile uint8_t lut_idx = 0;

static const uint8_t sine_lut[128] = {
    128, 134, 140, 146, 152, 158, 165, 170, 176, 182, 188, 193, 198, 203, 208, 213, 218, 222, 226, 230, 234, 237,
    240, 243, 245, 248, 250, 251, 253, 254, 254, 255, 255, 255, 254, 254, 253, 251, 250, 248, 245, 243, 240, 237,
    234, 230, 226, 222, 218, 213, 208, 203, 198, 193, 188, 182, 176, 170, 165, 158, 152, 146, 140, 134, 128, 121,
    115, 109, 103, 97,  90,  85,  79,  73,  67,  62,  57,  52,  47,  42,  37,  33,  29,  25,  21,  18,  15,  12,
    10,  7,   5,   4,   2,   1,   1,   0,   0,   0,   1,   1,   2,   4,   5,   7,   10,  12,  15,  18,  21,  25,
    29,  33,  37,  42,  47,  52,  57,  62,  67,  73,  79,  85,  90,  97,  103, 109, 115, 121};

void audio_enable(bool enable) {
  // Amp shutdown pin is active high, pull low to enable amplifier
  if (enable) {
    DL_TimerA_startCounter(TIMA0);
    DL_TimerA_setCaptureCompareValue(TIMA0, 127, DL_TIMER_CC_0_INDEX); // mid-scale
    DL_GPIO_clearPins(AMP_SD.port, AMP_SD.pinmask);
  } else {
    DL_GPIO_setPins(AMP_SD.port, AMP_SD.pinmask);
    DL_TimerA_stopCounter(TIMA0);
  }
}

void audio_init(void) {

  // Power up & reset the timer
  DL_TimerA_reset(TIMA0);
  DL_TimerA_enablePower(TIMA0);

  // Disable amplifier
  DL_GPIO_initDigitalOutput(AMP_SD.iomux);
  DL_GPIO_enableOutput(AMP_SD.port, AMP_SD.pinmask);
  DL_GPIO_setPins(AMP_SD.port, AMP_SD.pinmask);

  // Initialize GPIO to alternate function
  DL_GPIO_initPeripheralOutputFunction(AUDIO.iomux, IOMUX_PINCM29_PF_TIMA0_CCP0);

  // Setup TIMA0 for ~93kHz 8-bit PWM
  // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
  // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
  DL_TimerA_ClockConfig gClockConfig = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
      .prescale = 0U,
  };
  DL_TimerA_setClockConfig(TIMA0, (DL_TimerA_ClockConfig *)&gClockConfig);

  // Setup capture/compare channels for PWM
  DL_TimerA_PWMConfig pwmConfig = {
      .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
      .period = 255,
      .isTimerWithFourCC = true, // TIMA0 has 4 CC channels, we're only using one though.
      .startTimer = DL_TIMER_STOP,
  };
  DL_TimerA_initPWMMode(TIMA0, &pwmConfig);

  // CC0 = Audio
  DL_TimerA_setCCPDirection(TIMA0, DL_TIMER_CC0_OUTPUT);
  DL_TimerA_setCaptureCompareValue(TIMA0, 127, DL_TIMER_CC_0_INDEX); // mid-scale

  // Setup audio advance with period repeat counter
  // Update rate of ~46.9kHz
  DL_TimerA_setCounterRepeatMode(TIMA0, DL_TIMER_REPEAT_MODE_ENABLED);
  DL_TimerA_setRepeatCounter(TIMA0, 10);

  DL_TimerA_enableInterrupt(TIMA0, DL_TIMERA_INTERRUPT_REPC_EVENT);
  NVIC_EnableIRQ(TIMA0_INT_IRQn);

  DL_TimerA_enableClock(TIMA0);
  // DL_TimerA_startCounter(TIMA0);
}

void TIMA0_IRQHandler(void) {
  DL_TIMER_IIDX trigger = DL_TimerA_getPendingInterrupt(TIMA0);
  DL_TimerA_clearInterruptStatus(TIMA0, trigger);
  NVIC_ClearPendingIRQ(TIMA0_INT_IRQn);
  lut_idx += 1;
  DL_TimerA_setCaptureCompareValue(TIMA0, (sine_lut[lut_idx] / 2) + 64,
                                   DL_TIMER_CC_0_INDEX); // Full range is 8-bit, allow overflow
}