#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

#include <signal.h>

void get_args(int argc, char *argv[]);

int read_key_from_pipe (int pipe_des);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

char* determine_action(int pressed_key,  char *shared_act);

#endif // KEY_MANAGER_H