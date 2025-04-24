#define NSAMPLES 16
#define HSAMPLES 8

extern volatile bool fill_dac1;
extern volatile bool fill_dac2;

extern uint16_t dac_samples[];

void task_usb(void *arg);
