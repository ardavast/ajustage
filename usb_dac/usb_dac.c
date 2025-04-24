#include <string.h>
#include <stdio.h>
#include <math.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include <FreeRTOS.h>
#include <task.h>

#include "usb.h"


volatile bool fill_dac1 = false;
volatile bool fill_dac2 = false;

uint16_t dac_samples[NSAMPLES];

void task_dac(void *arg);

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	// FXS control pins
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12); //LED
	gpio_set(GPIOC, GPIO12);

	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7); // F/R
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9); // RM
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT,  GPIO_PUPD_NONE, GPIO0); // SHK
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5); // PD

	gpio_set(GPIOB, GPIO5);   // unsuspend
	gpio_clear(GPIOB, GPIO9); // turn off ringer
	gpio_set(GPIOB, GPIO7);   // normal polarity

	// TIM2 ticks at 8kHz
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_set_period(TIM2, 10500-1); // 84MHz / 10500 = 8kHz
	//timer_set_period(TIM2, 2625-1); // 84MHz / 2525 = 32kHz
	//timer_set_period(TIM2, 1750-1); // 84MHz / 1750 = 48kHz
	timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
	timer_enable_counter(TIM2);

	// PA5 is DAC channel 2
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);

	// DAC DMA
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);
	dma_stream_reset(DMA1, DMA_STREAM6);
	dma_set_transfer_mode(DMA1, DMA_STREAM6, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t)dac_samples);
	dma_set_memory_size(DMA1, DMA_STREAM6, DMA_SxCR_MSIZE_16BIT);
	dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t)&DAC_DHR12R2(DAC1));
	dma_set_peripheral_size(DMA1, DMA_STREAM6, DMA_SxCR_PSIZE_16BIT);
	dma_set_number_of_data(DMA1, DMA_STREAM6, NSAMPLES);
	dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_HIGH);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);
	dma_enable_circular_mode(DMA1, DMA_STREAM6);
	dma_enable_half_transfer_interrupt(DMA1, DMA_STREAM6);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
	dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM6);

	// DAC
	rcc_periph_clock_enable(RCC_DAC);
	dac_set_trigger_source(DAC1, DAC_CR_TSEL2_T2);
	dac_trigger_enable(DAC1, DAC_CHANNEL2);
	dac_dma_enable(DAC1, DAC_CHANNEL2);
	dac_enable(DAC1, DAC_CHANNEL2);

	xTaskCreate(task_usb, "task_usb", 2048, NULL, configMAX_PRIORITIES-1, NULL);

	vTaskStartScheduler();
	while (true);

	return 0;
}

void dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_HTIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_HTIF);
		fill_dac1 = true;
	}

	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
		fill_dac2 = true;
	}
}

void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
		                   char *pcTaskName)
{
	gpio_clear(GPIOC, GPIO12);
	while (1);
}
