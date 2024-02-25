#include "key_manager.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>

// Global variables for pipe descriptors to handle inter-process communication
// Pipes working with the server
int key_pressing_read;
// Pipes working with the server
int km_server_write;

int main(int argc, char *argv[])
{
    // Initialize the program by parsing command line arguments
    get_args(argc, argv);

    // Set up signal handling for SIGINT and SIGUSR1
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; // Signal handler function
    sa.sa_flags = SA_SIGINFO;   // Use sa_sigaction instead of sa_handler
    sigaction (SIGINT, &sa, NULL);  // Register signal handler for SIGINT
    sigaction (SIGUSR1, &sa, NULL); // Register signal handler for SIGUSR1

    // Publish this process's PID to the watchdog using a utility function
    publish_pid_to_wd(KM_SYM, getpid());

    // Main loop to process key presses indefinitely
    while(1)
    {
        /*THIS SECTION IS FOR OBTAINING THE KEY INPUT CHARACTER*/

        fd_set readset_km;
        // Initializes the file descriptor set readset by clearing all file descriptors from it.
        FD_ZERO(&readset_km);
        // Adds key_pressing to the file descriptor set readset.
        FD_SET(key_pressing_read, &readset_km);

        int ready;
        // This waits until a key press is sent from (interface.c)
        do
        {
            ready = select(key_pressing_read + 1, &readset_km, NULL, NULL, NULL);
        } while (ready == -1 && errno == EINTR);

        // Read from the file descriptor
        int pressed_key = read_key_from_pipe(key_pressing_read);
        printf("Pressed key: %c\n", (char)pressed_key); // Display the pressed key


        /*THIS SECTION IS FOR DRONE ACTION DECISION*/

        char *action = determine_action(pressed_key);
        // printf("Action sent to drone: %s\n\n", action);
        fflush(stdout);

        /*
        char key = toupper(pressed_key);
        int x; int y;
        // char action_msg[20];
        */

        // If the action is not "None", send it to the server
        if (strcmp(action, "None") != 0) 
        {
            write_to_pipe(km_server_write, action);
            printf("Action sent to server: %s\n", action);
        }         
    }

    return 0;

}

// Function to read a single character from a pipe
int read_key_from_pipe (int pipe_des)
{
    char msg[MSG_LEN];  // Buffer to hold the message

    ssize_t bytes_read = read(pipe_des, msg, sizeof(msg));  // Read from the pipe

    int pressed_key = msg[0];
    return pressed_key; // Return the first character read
}

// Parse the command line arguments to get pipe descriptors
void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &key_pressing_read, &km_server_write);
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        close(key_pressing_read);
        close(km_server_write);
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}


// Determine the action string based on the pressed key
char* determine_action(int pressed_key) 
{
    // Convert the pressed key to uppercase to handle both cases
    char key = toupper(pressed_key);
    // Switch on the uppercase character to determine the action
    switch (key) 
    {
        case 'W': return "0,-1";
        case 'X': return "0,1";
        case 'A': return "-1,0";
        case 'D': return "1,0";
        case 'Q': return "-1,-1";
        case 'E': return "1,-1";
        case 'Z': return "-1,1";
        case 'C': return "1,1";
        case 'S': return "10,0"; // Special value interpreted by drone.c process
        case 'P': return "STOP"; // Special value interpreted by server.c process
        default: return "None"; // If norecognized key, return "None" indicating no action
    }
}

// Utility function to log a message with a given log level
void log_msg(char *msg)
{
    // Sends a message to logger with the 'INFO' log level
    write_message_to_logger(KM_SYM, INFO, msg);
}