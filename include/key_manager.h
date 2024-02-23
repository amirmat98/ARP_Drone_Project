#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

#include <signal.h>

/**
 * get_args - This function is used to get the arguments passed to the program
 * @argc: Number of arguments passed to the program
 * @argv: Array of arguments passed to the program
 * 
 * This function is used to get the arguments passed to the program. It takes in the number of arguments passed to the program and an array of arguments. It then loops through the arguments and prints them out.
 */
void get_args(int argc, char *argv[]);

/**
 * read_key_from_pipe - This function is used to read a key from a pipe
 * @pipe_des: The descriptor of the pipe to read from
 * 
 * This function is used to read a key from a pipe. It takes in the descriptor of the pipe to read from. It then waits for a key to be written to the pipe, and returns the key that was read.
 * 
 * Returns: The key that was read from the pipe
 */
int read_key_from_pipe (int pipe_des);

/**
 * signal_handler - This function is used to handle signals sent to the program
 * @signo: The signal number that was sent to the program
 * @siginfo: Struct containing information about the signal
 * @context: Context information about the signal
 * 
 * This function is used to handle signals sent to the program. It takes in the signal number that was sent to the program, a struct containing information about the signal, and context information about the signal. It then determines the appropriate action to take based on the signal number and performs the action.
 */
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * determine_action - This function is used to determine the action to take based on the key that was pressed
 * @pressed_key: The key that was pressed
 * 
 * This function is used to determine the action to take based on the key that was pressed. It takes in the key that was pressed and returns the action to take.
 * 
 * Returns: The action to take based on the key that was pressed
 */
char* determine_action(int pressed_key);

/**
 * log_msg - This function is used to log a message to the console
 * @msg: The message to log
 * 
 * This function is used to log a message to the console. It takes in the message to log and prints it to the console.
 */
void log_msg(char *msg);

#endif // KEY_MANAGER_H