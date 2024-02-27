#ifndef MAIN_H    
#define MAIN_H

#include <stdio.h>
#include <signal.h>


int create_child(const char *program, char **arg_list);

void clean_up();

void get_args(int argc, char *argv[], char *program_type, char *socket_data);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void create_logfile();


#endif // MAIN_H