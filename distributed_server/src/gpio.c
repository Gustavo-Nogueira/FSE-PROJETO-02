#include "gpio.h"

#include <bcm2835.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_GPIO 32
#define BUFFER_MAX 50

static INTERRUPT_HANDLER interrupt_handlers[MAX_GPIO];
static pthread_mutex_t gpio_mutex;
static int gpio_isr = -1;
static int gpio_fd[MAX_GPIO] = {0};
static struct timeval gpio_last_change[MAX_GPIO];

/**
 * Open the GPIO files and set the input interrupt handler.
*/
int init_gpios(SERVER_CONFIG *sconfig, INTERRUPT_HANDLER handler) {
    for (int i = 0; i < sconfig->size_inputs; i++) {
        if (open_gpio(sconfig->inputs[i].gpio, 0) < 0) {
            return -1;
        }
        if (isr_gpio(sconfig->inputs[i].gpio, "both", handler) < 0) {
            return -2;
        }
    }

    for (int i = 0; i < sconfig->size_outputs; i++) {
        if (open_gpio(sconfig->outputs[i].gpio, 1) < 0) {
            return -3;
        }
    }

    return 0;
}

/**
 * Closes open GPIO files.
*/
void close_gpios() {
    for (int i = 0; i < MAX_GPIO; i++) {
        if (gpio_fd[i] > 0) {
            close(gpio_fd[i]);
        }
    }
}

/**
 * Opens communication to a gpio.
 * Direction: 1 for output; 0 for input.
*/
int open_gpio(int gpio, int direction) {
    if (gpio < 0 || gpio > (MAX_GPIO - 1)) return -1;
    if (direction < 0 || direction > 1) return -2;

    int len;
    char buf[BUFFER_MAX];

    if (gpio_fd[gpio] != 0) {
        close(gpio_fd[gpio]);
        gpio_fd[gpio] = open("/sys/class/gpio/unexport", O_WRONLY);
        len = snprintf(buf, BUFFER_MAX, "%d", gpio);
        write(gpio_fd[gpio], buf, len);
        close(gpio_fd[gpio]);
        gpio_fd[gpio] = 0;
    }

    gpio_fd[gpio] = open("/sys/class/gpio/export", O_WRONLY);
    len = snprintf(buf, BUFFER_MAX, "%d", gpio);
    write(gpio_fd[gpio], buf, len);
    close(gpio_fd[gpio]);

    len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/direction", gpio);
    gpio_fd[gpio] = open(buf, O_WRONLY);
    if (direction == 1) {
        write(gpio_fd[gpio], "out", 4);
        close(gpio_fd[gpio]);
        len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        gpio_fd[gpio] = open(buf, O_WRONLY);

    } else {
        write(gpio_fd[gpio], "in", 3);
        close(gpio_fd[gpio]);
        len = snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", gpio);
        gpio_fd[gpio] = open(buf, O_RDONLY);
    }

    return 0;
}

/**
 * Write a value(0 or 1) in a gpio.
*/
int write_gpio(int gpio, int value) {
    if (value == 0) {
        if (write(gpio_fd[gpio], "0", 1) < 0) {
            return -1;
        }
    } else {
        if (write(gpio_fd[gpio], "1", 1) < 0) {
            return -2;
        }
    }

    lseek(gpio_fd[gpio], 0, SEEK_SET);
    return 0;
}

/**
 * Reads a value(0 or 1) of a gpio.
*/
int read_gpio(int gpio) {
    char value_str[3];
    read(gpio_fd[gpio], value_str, 3);
    lseek(gpio_fd[gpio], 0, SEEK_SET);

    if (value_str[0] == '0') {
        return 0;
    } else {
        return 1;
    }
}

/**
 * Defines the type(s) of gpio events to be expected.
*/
static int set_edge_gpio(int gpio, char *edge) {
    char buf[BUFFER_MAX];
    snprintf(buf, BUFFER_MAX, "/sys/class/gpio/gpio%d/edge", gpio);

    int fd = open(buf, O_WRONLY);

    write(fd, edge, strlen(edge) + 1);

    close(fd);

    return 0;
}

/**
 * Find the time difference in usec.
*/
static unsigned long calc_delta_time(struct timeval *a, struct timeval *b) {
    return abs((a->tv_sec * 1000000 + a->tv_usec) - (b->tv_sec * 1000000 + b->tv_usec));
}

/**
 * Waits for interruption and calls the respective gpio handler.
*/
static void *wait_interruptions(void *arg) {
    unsigned long delta_time;
    int gpio = gpio_isr;
    struct pollfd fdset[1];
    struct timeval now;

    gpio_isr = -1;

    gettimeofday(&gpio_last_change[gpio], NULL);

    for (;;) {
        fdset[0].fd = gpio_fd[gpio];
        fdset[0].events = POLLPRI;
        fdset[0].revents = 0;

        int rc = poll(fdset, 1, -1);
        if (rc < 0) {
            pthread_exit(NULL);
        }

        if (fdset[0].revents & POLLPRI) {
            lseek(gpio_fd[gpio], 0, SEEK_SET);
            int val = read_gpio(gpio);

            gettimeofday(&now, NULL);
            delta_time = calc_delta_time(&now, &gpio_last_change[gpio]);

            interrupt_handlers[gpio](gpio, val, delta_time);

            gpio_last_change[gpio] = now;
        }
    }
}

/**
 * Configure a Interrupt Handler(ISR) for a gpio.
*/
int isr_gpio(int gpio, char *edge, INTERRUPT_HANDLER handler) {
    pthread_t isr_tid;

    if (gpio < 0 || gpio > (MAX_GPIO - 1) || gpio_fd[gpio] < 0) {
        return -1;
    }

    set_edge_gpio(gpio, edge);

    interrupt_handlers[gpio] = handler;

    pthread_mutex_lock(&gpio_mutex);

    gpio_isr = gpio;
    pthread_create(&isr_tid, NULL, &wait_interruptions, &gpio_isr);

    while (gpio_isr != -1) usleep(1000000);

    pthread_mutex_unlock(&gpio_mutex);

    return 0;
}
