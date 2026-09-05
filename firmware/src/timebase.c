#include "timebase.h"

static volatile uint32_t rollover_ms = 0;

uint32_t get_timestamp_ms() {
  // Build 32-bit value from rollover_ms and 16-bit timer value
  // Left shift timer value by 4 to get ms from 16kHz ticks
  // Overflows are handled by ms_have_elapsed()
  return rollover_ms + (DL_TimerA_getTimerCount(TIMA0) >> 4);
}

bool ms_have_elapsed(uint32_t original_time, uint32_t duration_ms) {
  uint32_t now = get_timestamp_ms();
  return (now - original_time) >= duration_ms; // ok with overflow
}

void TIMA0_IRQHandler() {
  DL_TIMER_IIDX trigger = DL_Timer_getPendingInterrupt(TIMA0);
  DL_Timer_clearInterruptStatus(TIMA0, trigger);
  NVIC_ClearPendingIRQ(TIMA0_INT_IRQn);
  rollover_ms += 4096; // 65536 ticks / 16 ticks-per-ms (allowed to overflow)
}

void init_timebase() {
  // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
  // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
  //   16.042 kHz = 24000000 Hz / (8 * (186 + 1))
  // Results in ~0.27% timing error
  DL_TimerA_ClockConfig gTickTIMClockConfig = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
      .prescale = 186U,
  };

  // Upcounting, roll over at full 16-bit value, set timer enable
  DL_TimerA_TimerConfig gTickTIMTimerConfig = {
      .period = 65535U,
      .timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
      .startTimer = DL_TIMER_START,
  };

  DL_TimerA_enablePower(TIMA0);
  DL_TimerA_setClockConfig(TIMA0, (DL_TimerA_ClockConfig *)&gTickTIMClockConfig);

  DL_TimerA_initTimerMode(TIMA0, (DL_TimerA_TimerConfig *)&gTickTIMTimerConfig);

  DL_TimerA_enableInterrupt(TIMA0, DL_TIMERA_INTERRUPT_ZERO_EVENT);
  NVIC_EnableIRQ(TIMA0_INT_IRQn);

  DL_TimerA_enableClock(TIMA0);
}