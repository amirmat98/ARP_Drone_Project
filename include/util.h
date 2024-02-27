#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>


void log_msg(char *file_path, char *who, char *message);

void log_err(char *file_path, char *who, char *message);

void publish_pid_to_wd(int process_symbol, pid_t pid);

void write_to_pipe(int pipe_des, char message[]);

// void write_message_to_logger(int who, int type, char *msg);

int read_pipe_non_blocking(int pipe_des, char message[]);

void read_and_echo(int socket_des, char socket_msg[]);

int read_and_echo_non_blocking(int socket_des, char socket_msg[], char *log_file, char *log_who);

void write_and_wait_echo(int socket_des, char socket_msg[], size_t msg_len, char *log_file, char *log_who);

void read_args_from_file(const char *file_name, char *type, char *data);

void parse_host_port(const char *str, char *host, int *port);

void block_signal(int signal);

void unblock_signal(int signal);

#endif // UTIL_H