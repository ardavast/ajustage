#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

#include <FreeRTOS.h>
#include <task.h>

#define msleep(x) vTaskDelay(pdMS_TO_TICKS((x)))
#define sleep(x) vTaskDelay(pdMS_TO_TICKS((x * 1000)))

#define BUT GPIOA, GPIO0
#define RM GPIOA, GPIO5
#define FR GPIOA, GPIO6

static void ring(int len, int freq) {
	gpio_set(RM);
	for (int i = 0; i < len * freq; i++) {
		gpio_clear(FR);
			sleep(0.5 / freq);
		gpio_set(FR);
		if (i != len * freq - 1) {
			sleep(0.5 / freq);
		}
	}
	gpio_clear(RM);
}

static void task1(void *args) {
	for (;;) {
		if (!gpio_get(BUT)) {
			sleep(10);
			ring(1, 20);
			sleep(10);
		}
	}
}

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5 | GPIO6);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO5 | GPIO6);
	gpio_clear(RM);
	gpio_set(FR);

	xTaskCreate(task1, "led", 100, NULL, configMAX_PRIORITIES-1, NULL);
        vTaskStartScheduler();

	while (1);

	return 0;
}

void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
		                   char *pcTaskName)
{
	while (1);
}
