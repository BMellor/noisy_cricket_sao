#pragma once

#include <ti/devices/msp/msp.h>
#include <ti/devices/msp/peripherals/hw_gpio.h>
#include <stdint.h>


typedef struct {
  GPIO_Regs* port;
  const uint32_t pinmask;
  const uint32_t iomux;
} gpio_pin_t;

const gpio_pin_t SDA      = {GPIOA, (1 << 0) , IOMUX_PINCM1 }; // PA0  (I2C0_SDA[3])
const gpio_pin_t SCL_NRST = {GPIOA, (1 << 1) , IOMUX_PINCM2 }; // PA1  (I2C0_SCL[2] / NRST)
const gpio_pin_t PDM_CLK  = {GPIOA, (1 << 17), IOMUX_PINCM18}; // PA17 (SPI0_SCK[4])
const gpio_pin_t SWDIO    = {GPIOA, (1 << 19), IOMUX_PINCM20}; // PA19
const gpio_pin_t SWCLK    = {GPIOA, (1 << 20), IOMUX_PINCM21}; // PA20
const gpio_pin_t PDM_DATA = {GPIOA, (1 << 22), IOMUX_PINCM23}; // PA22 (SPI0_POCI[3] i.e. MISO)
const gpio_pin_t LED_R    = {GPIOA, (1 << 23), IOMUX_PINCM24}; // PA23 (TIMG14_C0[4])
const gpio_pin_t LED_G    = {GPIOA, (1 << 24), IOMUX_PINCM25}; // PA24 (TIMG14_C1[3])
const gpio_pin_t LED_B    = {GPIOA, (1 << 25), IOMUX_PINCM26}; // PA25 (TIMG14_C3[2])
const gpio_pin_t AMP_SD   = {GPIOA, (1 << 26), IOMUX_PINCM27}; // PA27
const gpio_pin_t AUDIO    = {GPIOA, (1 << 28), IOMUX_PINCM29}; // PA28 (TIMA0_C0[2])