#ifndef WATCHDOG_H
#define WATCHDOG_H

#include<sys/types.h>

#define THRESHOLD 50 // Threshold for counters


int get_pids(pid_t *server_pid, pid_t *window_pid, pid_t *km_pid, pid_t *drone_pid);

void send_sigint_to_all();

#endif //WATCHDOG_H