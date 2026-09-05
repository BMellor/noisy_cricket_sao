#include "timebase.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_timerg.h>

static volatile uint32_t rollover_ms = 0;

uint32_t get_timestamp_ms(void) {
  // Build 32-bit value from rollover_ms and 16-bit timer value
  // Left shift timer value by 4 to get ms from 16kHz ticks
  // Overflows are handled by ms_have_elapsed()
  return rollover_ms + (DL_TimerG_getTimerCount(TIMG8) >> 4);
}

bool ms_have_elapsed(uint32_t original_time, uint32_t duration_ms) {
  uint32_t now = get_timestamp_ms();
  return (now - original_time) >= duration_ms; // ok with overflow
}

void delay_ms(uint32_t duration_ms) {
  uint32_t original_time = get_timestamp_ms();
  while (!ms_have_elapsed(original_time, duration_ms + 1)) {
    // Can't use WFE/WFI as this timer only wakes up the system every ~4 seconds
    //__WFE();
  }
}

void TIMG8_IRQHandler(void) {
  DL_TIMER_IIDX trigger = DL_Timer_getPendingInterrupt(TIMG8);
  DL_Timer_clearInterruptStatus(TIMG8, trigger);
  NVIC_ClearPendingIRQ(TIMG8_INT_IRQn);
  rollover_ms += 4096; // 65536 ticks / 16 ticks-per-ms (allowed to overflow)
}

void timebase_init(void) {
  // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
  // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
  //   16.042 kHz = 24000000 Hz / (8 * (186 + 1))
  // Results in ~0.27% timing error
  DL_TimerG_ClockConfig gTickTIMClockConfig = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
      .prescale = 186U,
  };

  // Upcounting, roll over at full 16-bit value, set timer enable
  DL_TimerG_TimerConfig gTickTIMTimerConfig = {
      .period = 65535U,
      .timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
      .startTimer = DL_TIMER_START,
  };

  DL_TimerG_enablePower(TIMG8);
  DL_TimerG_setClockConfig(TIMG8, (DL_TimerG_ClockConfig *)&gTickTIMClockConfig);

  DL_TimerG_initTimerMode(TIMG8, (DL_TimerG_TimerConfig *)&gTickTIMTimerConfig);

  DL_TimerG_enableInterrupt(TIMG8, DL_TIMERG_INTERRUPT_ZERO_EVENT);
  NVIC_EnableIRQ(TIMG8_INT_IRQn);

  DL_TimerG_enableClock(TIMG8);
}