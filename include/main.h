#ifndef MAIN_H    
#define MAIN_H


int create_child(const char *program, char **arg_list);

void close_all_pipes();

void signal_handler(int signo, siginfo_t *siginfo, void *context);


#endif // MAIN_H