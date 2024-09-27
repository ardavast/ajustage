#include <math.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>

#include <FreeRTOS.h>
#include <task.h>

#define TWOPI 6.2831854f

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	// PA4 is DAC channel 1
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);

	// TIM2 ticks at 8kHz
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_set_period(TIM2, 10500-1); // 84MHz / 10500 = 8kHz
	timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
	timer_enable_irq(TIM2, TIM_DIER_UIE);
	timer_enable_counter(TIM2);

	// DAC is triggered by TIM2
	rcc_periph_clock_enable(RCC_DAC);
	dac_set_trigger_source(DAC1, DAC_CR_TSEL1_T2);
	dac_trigger_enable(DAC1, DAC_CHANNEL1);
	dac_enable(DAC1, DAC_CHANNEL1);

	while (1);

	return 0;
}

void tim2_isr(void)
{
	static float amp = 0.3f;
	static float phi = 0.0f;

	if (timer_get_flag(TIM2, TIM_SR_UIF)) {
		timer_clear_flag(TIM2, TIM_SR_UIF);
		dac_load_data_buffer_single(DAC1,
		  (uint16_t)(amp * sinf(phi) * 2047.5f + 2047.5f),
		  DAC_ALIGN_RIGHT12, DAC_CHANNEL1);

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
