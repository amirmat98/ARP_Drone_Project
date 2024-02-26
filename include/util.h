#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>


void publish_pid_to_wd(char process_symbol, pid_t pid);

void write_to_pipe(int pipe_des, char message[]);

void write_message_to_logger(int who, int type, char *msg);

void error(char *msg);

int read_pipe_non_blocking(int pipe_des, char *message[]);

void read_and_echo(int socket, char socket_msg[]);

int read_and_echo_non_blocking(int socket, char socket_msg[]);

void write_and_wait_echo(int socket, char socket_msg[], size_t msg_len);

void read_args_from_file(const char *file_name, char *type, char *data);

void parse_host_port(const char *str, char *host, int *port);

#endif // UTIL_H