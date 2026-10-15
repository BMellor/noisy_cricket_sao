#include "i2c_target.h"
#include "config.h"
#include "pins.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_i2c.h>

static uint8_t regs[REG_COUNT];
static uint8_t ptr;
static bool have_ptr;
static bool got_data;
static bool got_beep;
static volatile bool host_wrote, beep_requested;

static bool writable(uint8_t r) {
  return r == REG_THRESHOLD || r == REG_THRESHOLD + 1 || r == REG_COMMIT || r == REG_VOLUME ||
         (r >= REG_LED && r < REG_LED + 3) || (r >= REG_BEEP_FREQ && r < REG_BEEP_DUR + 2);
}

// Lowest priority. One byte is loaded per TX request (TX FIFO empty while the host is clock-stretched),
// so nothing is prefetched and the register pointer always matches what the host has read.
void I2C0_IRQHandler(void) {
  switch (DL_I2C_getPendingInterrupt(I2C0)) {
  case DL_I2C_IIDX_TARGET_START:
    have_ptr = false;
    DL_I2C_flushTargetTXFIFO(I2C0);
    break;
  case DL_I2C_IIDX_TARGET_RXFIFO_TRIGGER:
    while (!DL_I2C_isTargetRXFIFOEmpty(I2C0)) {
      uint8_t b = DL_I2C_receiveTargetData(I2C0);
      if (!have_ptr) {
        ptr = b;
        have_ptr = true;
      } else {
        if (ptr < REG_COUNT && writable(ptr)) {
          regs[ptr] = b;
          got_data = true;
          if (ptr == REG_BEEP_DUR || ptr == REG_BEEP_DUR + 1)
            got_beep = true;
        }
        ptr++;
      }
    }
    break;
  case DL_I2C_IIDX_TARGET_TXFIFO_EMPTY:
    DL_I2C_transmitTargetData(I2C0, ptr < REG_COUNT ? regs[ptr] : 0xFF);
    ptr++;
    break;
  case DL_I2C_IIDX_TARGET_STOP:
    if (got_data) {
      got_data = false;
      host_wrote = true;
    }
    if (got_beep) {
      got_beep = false;
      beep_requested = true;
    }
    DL_I2C_flushTargetTXFIFO(I2C0);
    break;
  default:
    break;
  }
}

void i2c_target_init(void) {
  regs[REG_ID] = REG_ID_VALUE;

  DL_I2C_reset(I2C0);
  DL_I2C_enablePower(I2C0);
  DL_Common_delayCycles(16);

  // PA1 doubles as NRST; main() has already disabled the reset function
  DL_GPIO_initPeripheralInputFunctionFeatures(SDA.iomux, IOMUX_PINCM1_PF_I2C0_SDA, DL_GPIO_INVERSION_DISABLE,
                                              DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
                                              DL_GPIO_WAKEUP_DISABLE);
  DL_GPIO_initPeripheralInputFunctionFeatures(SCL_NRST.iomux, IOMUX_PINCM2_PF_I2C0_SCL, DL_GPIO_INVERSION_DISABLE,
                                              DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
                                              DL_GPIO_WAKEUP_DISABLE);
  DL_GPIO_enableHiZ(SDA.iomux);
  DL_GPIO_enableHiZ(SCL_NRST.iomux);

  DL_I2C_ClockConfig clkCfg = {
      .clockSel = DL_I2C_CLOCK_BUSCLK,
      .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
  };
  DL_I2C_setClockConfig(I2C0, &clkCfg);
  DL_I2C_enableAnalogGlitchFilter(I2C0);

  DL_I2C_setTargetOwnAddress(I2C0, I2C_TARGET_ADDR);
  DL_I2C_enableTargetOwnAddress(I2C0);
  DL_I2C_setTargetTXFIFOThreshold(I2C0, DL_I2C_TX_FIFO_LEVEL_BYTES_1);
  DL_I2C_setTargetRXFIFOThreshold(I2C0, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
  DL_I2C_enableTargetTXEmptyOnTXRequest(I2C0);
  DL_I2C_enableTargetTXWaitWhenTXFIFOStale(I2C0);
  DL_I2C_enableTargetClockStretching(I2C0);

  DL_I2C_enableInterrupt(I2C0, DL_I2C_INTERRUPT_TARGET_START | DL_I2C_INTERRUPT_TARGET_STOP |
                                   DL_I2C_INTERRUPT_TARGET_RXFIFO_TRIGGER | DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);
  NVIC_SetPriority(I2C0_INT_IRQn, 2);
  NVIC_EnableIRQ(I2C0_INT_IRQn);

  DL_I2C_enableTarget(I2C0);
}

void i2c_target_set(uint8_t reg, const uint8_t *src, uint8_t len) {
  NVIC_DisableIRQ(I2C0_INT_IRQn);
  for (uint8_t i = 0; i < len; i++)
    regs[reg + i] = src[i];
  NVIC_EnableIRQ(I2C0_INT_IRQn);
}

void i2c_target_get(uint8_t reg, uint8_t *dst, uint8_t len) {
  NVIC_DisableIRQ(I2C0_INT_IRQn);
  for (uint8_t i = 0; i < len; i++)
    dst[i] = regs[reg + i];
  NVIC_EnableIRQ(I2C0_INT_IRQn);
}

bool i2c_target_beep_requested(void) {
  if (!beep_requested)
    return false;
  beep_requested = false;
  return true;
}

bool i2c_target_host_wrote(void) {
  if (!host_wrote)
    return false;
  host_wrote = false;
  return true;
}
