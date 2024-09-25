#include <math.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>

#include <FreeRTOS.h>
#include <task.h>

#define TWOPI 6.2831854

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	// Initialize PA4 as DAC channel 1
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);

	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_reset_pulse(RST_TIM2);

	// Clock division ratio: 1
	// Center-aligned Mode Selection: Edge
	// Direction: Up
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_continuous_mode(TIM2); // Continous mode (that is, not one-shot mode, ~OPM)
	timer_set_period(TIM2, 10500-1); // 125us (8kHz)
	timer_enable_irq(TIM2, TIM_DIER_UIE);
	timer_enable_counter(TIM2);

	// DAC
	rcc_periph_clock_enable(RCC_DAC);
	dac_trigger_enable(DAC1, DAC_CHANNEL1);
	dac_set_trigger_source(DAC1, DAC_CR_TSEL1_SW);
	dac_enable(DAC1, DAC_CHANNEL1);

	while (1);

	return 0;
}

void tim2_isr(void)
{
	static int n;
	static float phi;

	if (timer_get_flag(TIM2, TIM_SR_UIF)) {
		timer_clear_flag(TIM2, TIM_SR_UIF);
		dac_load_data_buffer_single(DAC1, 
		  (uint8_t)(0.2f * (sinf(phi)) * 127.5f + 127.5f),
		  DAC_ALIGN_RIGHT8, DAC_CHANNEL1);
		dac_software_trigger(DAC1, DAC_CHANNEL1);

		phi += TWOPI * 440.0f / 8000.0f;
		if (phi > TWOPI) {
			phi -= TWOPI;
		}
	}
}

void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
		                   char *pcTaskName)
{
	while (1);
}
