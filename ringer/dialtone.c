#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#define TWOPI 6.2831854f

#define NSAMPLES 256
#define HSAMPLES 128

static volatile bool fill1 = false;
static volatile bool fill2 = false;
static uint8_t samples[NSAMPLES];

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);

	// LED
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_set(GPIOC, GPIO12);

	// PA9 is UART_TX
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9);

	// PA10 is UART_RX
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO10);

	// UART
	rcc_periph_clock_enable(RCC_USART1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);

	// PA5 is DAC channel 2
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);

	// TIM2 ticks at 8kHz
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_set_period(TIM2, 10500-1); // 84MHz / 10500 = 8kHz
	timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
	timer_enable_counter(TIM2);

	// DMA
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);
	dma_stream_reset(DMA1, DMA_STREAM6);
	dma_set_transfer_mode(DMA1, DMA_STREAM6, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t)samples);
	dma_set_memory_size(DMA1, DMA_STREAM6, DMA_SxCR_MSIZE_8BIT);
	dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t)&DAC_DHR8R2(DAC1));
	dma_set_peripheral_size(DMA1, DMA_STREAM6, DMA_SxCR_PSIZE_8BIT);
	dma_set_number_of_data(DMA1, DMA_STREAM6, NSAMPLES);
	dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_HIGH);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);
	dma_enable_circular_mode(DMA1, DMA_STREAM6);
	dma_enable_half_transfer_interrupt(DMA1, DMA_STREAM6);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM6);
	dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM6);

	// DAC is triggered by TIM2
	rcc_periph_clock_enable(RCC_DAC);
	dac_set_trigger_source(DAC1, DAC_CR_TSEL2_T2);
	dac_trigger_enable(DAC1, DAC_CHANNEL2);
	dac_dma_enable(DAC1, DAC_CHANNEL2);
	dac_enable(DAC1, DAC_CHANNEL2);

	for (;;) {
		while (!fill1);
		usart_send_blocking(USART1, HSAMPLES/8);
		for (int i = 0; i < HSAMPLES; i++) {
			samples[i] = usart_recv_blocking(USART1);
		}
		fill1 = false;

		while (!fill2);
		usart_send_blocking(USART1, HSAMPLES/8);
		for (int i = HSAMPLES; i < NSAMPLES; i++) {
			samples[i] = usart_recv_blocking(USART1);
		}
		fill2 = false;
	}

	return 0;
}

void dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_HTIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_HTIF);
		if (fill1) {
			gpio_toggle(GPIOC, GPIO12);
		}
		fill1 = true;
	}

	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
		if (fill2) {
			gpio_toggle(GPIOC, GPIO12);
		}
		fill2 = true;
	}
}
