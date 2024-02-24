#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

#define SLEEP_TIME 500000  

void get_args(int argc, char *argv[]);
void signal_handler(int signo, siginfo_t *siginfo, void *context);
void *create_shm(char *name);
void clean_up();

char* read_and_echo(int socket);
char* read_and_echo_non_blocking(int socket);

void write_message_and_wait_for_echo(int socket, char *message);

#endif // SERVER_H