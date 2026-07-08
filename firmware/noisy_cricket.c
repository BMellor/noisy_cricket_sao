/* Template app on which you can build your own. */

#include "ch32fun.h"
#include <stdio.h>

uint32_t count;

#define LED_R_PORT GPIOD
#define LED_R_PIN 2

#define LED_G_PORT GPIOD
#define LED_G_PIN 3

#define LED_B_PORT GPIOD
#define LED_B_PIN 4

#define UART_TX_PORT GPIOC
#define UART_TX_PIN 0

#define SDA_PORT GPIOC
#define SDA_PIN 1

#define SCL_PORT GPIOC
#define SCL_PIN 2

#define PDM_CLK_PORT GPIOC
#define PDM_CLK_PIN 5

#define PDM_DATA_PORT GPIOC
#define PDM_DATA_PIN 7

#define AUDIO_PORT GPIOA
#define AUDIO_PIN 1

#define AMP_SD_PORT GPIOA
#define AMP_SD_PIN 2

const uint8_t sine8[8] = {128, 218, 255, 218, 128, 37, 0, 37};
const uint8_t sine16[16] = {128, 176, 218, 245, 255, 245, 218, 176, 128, 79, 37, 10, 0, 10, 37, 79};
const uint8_t sine32[32] = {128, 131, 156, 180, 202, 221, 237, 247, 253, 254, 249, 240, 226, 208, 187, 163, 138,
   112, 87, 64, 43, 26, 12, 4, 0, 2, 8, 20, 35, 55, 77, 102 };
const uint8_t sine64[64] = {127, 139, 152, 164, 176, 187, 198, 208, 217, 225, 233, 239, 244, 249, 252, 253, 254,
	253, 252, 249, 244, 239, 233, 225, 217, 208, 198, 187, 176, 164, 152, 139, 127, 115, 102, 90, 78, 67, 56, 46,
	37, 29, 21, 15, 10, 5, 2, 1, 0, 1, 2, 5, 10, 15, 21, 29, 37, 46, 56, 67, 78, 90, 102, 115};

volatile uint8_t sine_idx = 0;

// Select the active wave table
#define WAVE_TABLE_NAME sine64
#define WAVE_TABLE_LENGTH 64

void set_freq(uint16_t freq_hz) {
	sine_idx = 0;
	// period = fclk/(freq_hz*wave_table_length)
	TIM2->ATRLR = 48000000/(freq_hz*WAVE_TABLE_LENGTH);
	TIM2->SWEVGR |= TIM_UG; // Reload timer immediately
}

// Apparently this is neccesary 
void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void)
{
	TIM2->INTFR &= ~TIM_UIF; // Clear ISR flag
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	sine_idx = (sine_idx < (WAVE_TABLE_LENGTH-1)) ? sine_idx+1 : 0;
	TIM1->CH2CVR = WAVE_TABLE_NAME[sine_idx] / 8;

	// AUDIO_PORT->OUTDR ^= (1<<AUDIO_PIN);
}

volatile uint8_t foo = 255;

// Apparently this is neccesary 
void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void)
{
	TIM1->INTFR &= ~TIM_UIF; // Clear ISR flag
	NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
	TIM1->CH2CVR = foo--; // allow roll-over
}

int main()
{
	SystemInit(); // Default 48Mhz internal osc, 1kHz SysTick (no IRQ)

	// Enable GPIOs
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

	// Disable audio amp (set high)
	AMP_SD_PORT->CFGLR &= ~(0xf<<(4*AMP_SD_PIN));
	AMP_SD_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*AMP_SD_PIN);
	AMP_SD_PORT->BSHR = (1<<AMP_SD_PIN);

	// // LED pins as GPIO push-pull outputs
	// // LED_R_PORT->CFGLR &= ~(0xf<<(4*LED_R_PIN));
	// // LED_R_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*LED_R_PIN);
	// // LED_G_PORT->CFGLR &= ~(0xf<<(4*LED_G_PIN));
	// // LED_G_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*LED_G_PIN);
	// // LED_B_PORT->CFGLR &= ~(0xf<<(4*LED_B_PIN));
	// // LED_B_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*LED_B_PIN);

	// // Setup TIM1 to generate PWM
	// RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
	
	// // // PA1 is T1CH2, 10MHz Output alt func, push-pull
	// AUDIO_PORT->CFGLR &= ~(0xf<<(4*AUDIO_PIN));
	// AUDIO_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*AUDIO_PIN);
		
	// // AUDIO_PORT->CFGLR &= ~(0xf<<(4*AUDIO_PIN));
	// // AUDIO_PORT->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*AUDIO_PIN);

	// // Reset TIM1 to init all regs
	// RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	// RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
	
	// // ---INVALID--- Auto Reload - sets period (48MHz fclk / 2 prescaler / 256 data / 16 wave table = 5859.375 Hz)

	// // TIM1 PWM Frequency (48MHz / 2x prescaler/ 256 data = 93.75kHz)
	// TIM1->PSC = 1;
	// TIM1->ATRLR = 255;
	
	// // Enable CH2 output, positive pol
	// TIM1->CCER |= TIM_CC2E | TIM_CC2P;

	// // CH2 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
	// TIM1->CHCTLR1 |= TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
	
	// // Set the Capture Compare Register to zero bias
	// TIM1->CH2CVR = 127;
	
	// // Reload immediately
	// TIM1->SWEVGR |= TIM_UG;

	// // Enable TIM1 outputs
	// TIM1->BDTR |= TIM_MOE;
	
	// // TIM1->DMAINTENR |= TIM_UIE; // Debugging only

	// // Enable TIM1
	// TIM1->CTLR1 |= TIM_ARPE | TIM_CEN;


	// // Setup TIM2 to advance the wave table
	// RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// // Reset TIM2 to init all regs
	// RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	// RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	
	// // Prescaler
	// TIM2->PSC = 0;
	
 	// set_freq(1000);
 
	// // TIM2->ATRLR = 10000;
	// // TIM2->SWEVGR |= TIM_UG; // Reload timer immediately
	
	// // Enable update interrupt
	// TIM2->DMAINTENR |= TIM_UIE;
	
	// // Enable TIM2
	// TIM2->CTLR1 |= TIM_CEN;

	// NVIC_EnableIRQ(TIM2_IRQn);

	// // NVIC_EnableIRQ(TIM1_UP_IRQn);

	// __enable_irq();

	// Enable audio amp
	//AMP_SD_PORT->BCR = (1<<AMP_SD_PIN);

	// uint16_t freq_hz = 100;
	while(1)
	{
		__WFI();
		// set_freq(freq_hz);
		// Delay_Ms(500);
		// freq_hz = (freq_hz < 5800) ? freq_hz + 100 : 100;


		// LED_R_PORT->BCR = (1<<LED_R_PIN); // Red On
		// Delay_Ms(3000); // Polling, doesn't use SysTick IRQ
		// LED_R_PORT->BSHR = (1<<LED_R_PIN); // Red Off
		// LED_G_PORT->BCR = (1<<LED_G_PIN); // Green On
		// Delay_Ms(3000);
		// LED_G_PORT->BSHR = (1<<LED_G_PIN); // Green Off
		// LED_B_PORT->BCR = (1<<LED_B_PIN); // Blue On
		// Delay_Ms(3000);
		// LED_B_PORT->BSHR = (1<<LED_B_PIN); // Blue Off
		// Delay_Ms(3000);
	}
}

