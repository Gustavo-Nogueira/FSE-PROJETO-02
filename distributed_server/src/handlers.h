#ifndef HANDLERS_H_
#define HANDLERS_H_

void *listen_requests_adapter(void *arg);

void handle_interruption(int gpio, int value, unsigned long delta_time);

void handle_sigint(int sig);

#endif /* HANDLERS_H_ */
