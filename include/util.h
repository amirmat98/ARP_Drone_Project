#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>


void publish_pid_to_wd(char process_symbol, pid_t pid);

void write_to_pipe(int pipe_des, char message[]);

void write_message_to_logger(int who, int type, char *msg);


#endif // UTIL_H