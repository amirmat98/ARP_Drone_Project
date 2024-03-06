#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>


/**
 * @brief This function is used to publish the process id to the watchdog process
 * 
 * @param process_symbol The process symbol to be used in the watchdog process
 * @param pid The process id to be published
 */
void publish_pid_to_wd(char process_symbol, pid_t pid);

/**
 * @brief This function is used to write a message to a pipe
 * 
 * @param pipe_des The descriptor of the pipe to write to
 * @param message The message to be written to the pipe
 */
void write_to_pipe(int pipe_des, char message[]);

/**
 * @brief This function is used to write a message to a pipe
 * 
 * @param pipe_des The descriptor of the pipe to write to
 * @param message The message to be written to the pipe
 */
void write_message_to_logger(int who, int type, char *msg);

/**
 * @brief This function is used to read a message from a pipe
 * 
 * @param pipe_des The descriptor of the pipe to read from
 * @param message The buffer to store the message read from the pipe
 * @return int The number of characters read from the pipe, or -1 if an error occurred
 */
int read_from_pipe(int pipe_des, char message[]);


#endif // UTIL_H