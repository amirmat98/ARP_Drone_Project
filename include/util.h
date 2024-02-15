#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>


void publish_pid_to_wd(char process_symbol, pid_t pid);

void write_to_pipe(int pipe_des, char *message);

char *read_from_pipe(int pipe_des);

void log_msg(int who, int type, char *msg);


#endif // UTIL_H