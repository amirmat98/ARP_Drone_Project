#include "key_manager.h"
#include "import.h"

// Global variables for pipe descriptors to handle inter-process communication
int interface_km;
int km_server;

char log_file[80];
char msg[1024];

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
        //////////////////////////////////////////////////////
        /* SECTION 1: READ DATA FROM INTERFACE */
        /////////////////////////////////////////////////////

        char interface_msg[MSG_LEN];
        ssize_t bytes_read = read(interface_km, interface_msg, MSG_LEN);
        char pressed_key = interface_msg[0];
        // printf("Pressed key: %c\n", pressed_key);
        sprintf(msg, "Pressed key: %c\n", pressed_key);
        log_msg(log_file, KM, msg);



        //////////////////////////////////////////////////////
        /* SECTION 2: DRONE ACTION SELECTION */
        /////////////////////////////////////////////////////

        char *action = determine_action(pressed_key);

        if (action != "None")
        {
            write_to_pipe(km_server, action);
            // printf("[PIPE] SENT to server.c: %s\n", action);
            sprintf(msg, "[PIPE] SENT to server.c: %s\n", action);
            log_msg(log_file, KM, msg);
        }
    }

    return 0;

}

// Parse the command line arguments to get pipe descriptors
void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %s", &interface_km, &km_server, log_file);
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT \n");
        log_msg(log_file, KM, msg);
        close(interface_km);
        close(km_server);
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
char* determine_action(char pressed_key) 
{
    // Convert the pressed key to uppercase to handle both cases
    char key = toupper(pressed_key);

    /* Returns are in the format "x,y" */ 
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