#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

void signal_handler(int signo, siginfo_t *siginfo, void *context);
void *create_shm(char *name);
void clean_up();

#endif // SERVER_H