#ifndef GPIO_H_
#define GPIO_H_

#include "definitions.h"

typedef void (*INTERRUPT_HANDLER)(int gpio, int value, unsigned long delta_time);

int init_gpios(SERVER_CONFIG *sconfig, INTERRUPT_HANDLER handler);

void close_gpios();

int isr_gpio(int gpio, char *edge, INTERRUPT_HANDLER handler);

int open_gpio(int gpio, int direction);

int write_gpio(int gpio, int value);

int read_gpio(int gpio);

#endif /* GPIO_H_ */
