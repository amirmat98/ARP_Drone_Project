#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

#define SLEEP_TIME 500000  

void get_args(int argc, char *argv[]);
void signal_handler(int signo, siginfo_t *siginfo, void *context);
void *create_shm(char *name);
void clean_up();

void read_and_echo(int socket, char socket_msg[]);
int read_and_echo_non_blocking(int socket, char socket_msg[]);
void write_message_and_wait_for_echo(int socket, char socket_msg[], size_t msg_size);
int read_pipe_non_blocking(int pipe, char msg[]);

#endif // SERVER_H