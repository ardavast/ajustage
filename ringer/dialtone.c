#include <math.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>

#include <FreeRTOS.h>
#include <task.h>

#define TWOPI 6.2831854f

static uint16_t samples[256];

void get_samples(uint16_t *buf) {
	static float amp = 0.3f;
	static float phi = 0.0f;

	for (int i = 0; i < 128; i++) {
		buf[i] = (uint16_t)(amp * sinf(phi) * 2047.5f + 2047.5f);
		phi += TWOPI * 440.0f / 8000.0f;
		if (phi > TWOPI) {
			phi -= TWOPI;
		}
	}
}

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	// PA4 is DAC channel 1
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);

	// TIM2 ticks at 8kHz
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_set_period(TIM2, 10500-1); // 84MHz / 10500 = 8kHz
	timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
	timer_enable_counter(TIM2);

	// DMA
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
	dma_stream_reset(DMA1, DMA_STREAM5);
	dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) samples);
	dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_16BIT);
	dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t)&DAC_DHR12R1(DAC1));
	dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_16BIT);
	dma_set_number_of_data(DMA1, DMA_STREAM5, 256);
	dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_HIGH);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
	dma_enable_circular_mode(DMA1, DMA_STREAM5);
	dma_enable_half_transfer_interrupt(DMA1, DMA_STREAM5);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
	dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM5);

	// DAC is triggered by TIM2
	rcc_periph_clock_enable(RCC_DAC);
	dac_set_trigger_source(DAC1, DAC_CR_TSEL1_T2);
	dac_trigger_enable(DAC1, DAC_CHANNEL1);
	dac_dma_enable(DAC1, DAC_CHANNEL1);
	dac_enable(DAC1, DAC_CHANNEL1);

	while (1);

	return 0;
}

void dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_HTIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_HTIF);
		get_samples(samples);
	}

	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		get_samples(samples+128);
	}
}

void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
		                   char *pcTaskName)
{
	while (1);
}
