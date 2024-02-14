#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>


void signal_handler(int signo, siginfo_t *siginfo, void *context);
void get_args(int argc, char *argv[]);

void draw_window(int max_x, int max_y, int drone_x, int drone_y);
void handle_input(int *shared_key, sem_t *semaphore);

char* determine_action(int pressed_key, char *shared_action);

#endif //INTERFACE_H