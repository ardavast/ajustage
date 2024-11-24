/*
 * Rings for one second when the button is pressed.
 * The waveform is described on page 7 of:
 * https://silvertel.com/images/datasheets/Ag1171-datasheet-Low-cost-ringing-SLIC-with-single-supply.pdf
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

#include <FreeRTOS.h>
#include <task.h>

#define msleep(x) vTaskDelay(pdMS_TO_TICKS((x)))
#define sleep(x) vTaskDelay(pdMS_TO_TICKS((x * 1000)))

#define PORT_BUT GPIOA
#define GPIO_BUT GPIO0

#define PORT_RM GPIOB
#define GPIO_RM GPIO9

#define PORT_FR GPIOB
#define GPIO_FR GPIO7

static void ringer(void *args) {
	for (;;) {
		if (!gpio_get(PORT_BUT, GPIO_BUT)) {
			gpio_set(PORT_RM, GPIO_RM);
			for (int i = 0; i < 20; i++) {
				gpio_clear(PORT_FR, GPIO_FR);
				sleep(0.5 / 20);
				gpio_set(PORT_FR, GPIO_FR);
				if (i != 19) {
					sleep(0.5 / 20);
				}
			}
			gpio_clear(PORT_RM, GPIO_RM);
		}
	}
}

int main(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_mode_setup(PORT_RM, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_RM);
	gpio_set_output_options(PORT_RM, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_RM);

	gpio_mode_setup(PORT_FR, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_FR);
	gpio_set_output_options(PORT_FR, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_FR);

	gpio_clear(PORT_RM, GPIO_RM);
	gpio_set(PORT_FR, GPIO_FR);

	xTaskCreate(ringer, "ringer", 100, NULL, configMAX_PRIORITIES-1, NULL);
	vTaskStartScheduler();

	while (1);

	return 0;
}

void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
		                   char *pcTaskName)
{
	while (1);
}
