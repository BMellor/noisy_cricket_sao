#include "timebase.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_timerg.h>

static volatile uint32_t timestamp_ms = 0;

uint32_t get_timestamp_ms(void) { return timestamp_ms; }

bool ms_have_elapsed(uint32_t original_time, uint32_t duration_ms) {
  return (timestamp_ms - original_time) >= duration_ms; // ok with overflow
}

void delay_ms(uint32_t duration_ms) {
  uint32_t original_time = get_timestamp_ms();
  while (!ms_have_elapsed(original_time, duration_ms + 1)) {
    __WFI();
  }
}

void TIMG8_IRQHandler(void) {
  DL_TIMER_IIDX trigger = DL_Timer_getPendingInterrupt(TIMG8);
  DL_Timer_clearInterruptStatus(TIMG8, trigger);
  NVIC_ClearPendingIRQ(TIMG8_INT_IRQn);
  timestamp_ms++;
}

void timebase_init(void) {
  // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
  // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
  //   16.042 kHz = 24000000 Hz / (8 * (186 + 1))
  // Results in ~0.27% timing error
  DL_TimerG_ClockConfig gTickTIMClockConfig = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
      .prescale = 0U,
  };

  DL_TimerG_TimerConfig gTickTIMTimerConfig = {
      .period = TIMEBASE_TICKS_PER_MS,
      .timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
      .startTimer = DL_TIMER_START,
  };

  DL_TimerG_enablePower(TIMG8);
  DL_TimerG_setClockConfig(TIMG8, (DL_TimerG_ClockConfig *)&gTickTIMClockConfig);

  DL_TimerG_initTimerMode(TIMG8, (DL_TimerG_TimerConfig *)&gTickTIMTimerConfig);

  DL_TimerG_enableInterrupt(TIMG8, DL_TIMERG_INTERRUPT_ZERO_EVENT);
  NVIC_SetPriority(TIMG8_INT_IRQn, 1);
  NVIC_EnableIRQ(TIMG8_INT_IRQn);

  DL_TimerG_enableClock(TIMG8);
}