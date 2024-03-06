#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

#define SLEEP_TIME 500000  

/**
 * get_args function takes in the number of arguments and the arguments themselves as input.
 * It then loops through each argument, checking if it is a flag and if it is, it sets the corresponding boolean value to true.
 * If the argument is not a flag, it sets the value of the argument to the value of the next argument.
 * The function returns 0 on success and -1 on failure.
 */
void get_args(int argc, char *argv[]);

/**
 * signal_handler function takes in the signal number as input and handles the signal accordingly.
 * The function sets the boolean value to true to indicate that the signal has been handled.
 * The function returns 0 on success and -1 on failure.
 */
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * create_shm function takes in the name of the shared memory segment as input.
 * It creates a shared memory segment with the given name, sets the permission to allow read and write access for the current user, and returns a pointer to the shared memory segment on success.
 * If the shared memory segment already exists, the function returns a pointer to the existing shared memory segment.
 * The function returns NULL on failure.
 */
void *create_shm(char *name);

/**
 * clean_up function is used to clean up any resources that were allocated during the execution of the program.
 * It is called at the end of the program to ensure that all resources are properly freed and released.
 */
void clean_up();

/**
 * shared_memory_handler function is used to handle the creation and deletion of a shared memory segment.
 * It takes in the name of the shared memory segment as input and creates the shared memory segment if it does not exist.
 * If the shared memory segment already exists, the function deletes it and creates a new one with the same name.
 * The function then attaches to the shared memory segment and maps the memory into the process's address space.
 * The function then sets up a signal handler to handle SIGINT and SIGTERM signals, which will clean up any resources and exit the program.
 * The function returns 0 on success and -1 on failure.
 */
void shared_memory_handler(char name);

#endif // SERVER_H