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

// GLOBAL VARIABLES
int shared_key;
int shared_action;
void *ptr_key;              // Shared memory for Key pressing
void *ptr_action;           // Shared memory for Drone Position      
sem_t *sem_key;             // Semaphore for key presses
sem_t *sem_action;          // Semaphore for drone positions

/*
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_action);

        printf("Succesfully closed all semaphores\n");
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
*/

// Pipes
int key_pressing[2];
int km_server[2];

int main(int argc, char *argv[])
{

    get_args(argc, argv);

    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);

    publish_pid_to_wd(KM_SYM, getpid());

    // Initialize shared memory for KEY PRESSING
    shared_key = shm_open(SHAREMEMORY_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shared_key, 0);

    // Initialize shared memory for DRONE CONTROL - ACTION
    shared_action = shm_open(SHAREMEMORY_ACTION, O_RDWR, 0666);
    ptr_action = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shared_action, 0);

    // Initialize semaphores
    sem_key = sem_open(SEMAPHORE_KEY, 0);
    sem_action = sem_open(SEMAPHORE_ACTION, 0);

 
    while(1)
    {
        /*THIS SECTION IS FOR OBTAINING THE KEY INPUT CHARACTER*/

        fd_set readset_km;
        // Initializes the file descriptor set readset by clearing all file descriptors from it.
        FD_ZERO(&readset_km);
        // Adds key_pressing to the file descriptor set readset.
        FD_SET(key_pressing[0], &readset_km);

        int ready;
         // This waits until a key press is sent from (interface.c)
         do
         {
            ready = select(key_pressing[0] + 1, &readset_km, NULL, NULL, NULL);
         } while (ready == -1 && errno == EINTR);

         // Read from the file descriptor
         int pressed_key = read_key_from_pipe(key_pressing[0]);
         printf("Pressed key: %c\n", (char)pressed_key);
         fflush(stdout);

         /*THIS SECTION IS FOR DRONE ACTION DECISION*/

         char *action = determine_action(pressed_key, ptr_action);
         printf("Action sent to drone: %s\n\n", action);
         fflush(stdout);

         // TEMPORAL/DELETE AFTER: TESTING DATA SENT TO PIPE ACTION
         char key = toupper(pressed_key);
         int x; int y;
         // char action_msg[20];

         if ( action != "None")
         {
            write_to_pipe (km_server[1], action);
            printf("Wrote action message: %s into pipe\n", action);
         }        
    }

    // DELETE: Everything related to shared memory and semaphores

    // close shared memories
    close(shared_key);
    close(shared_action);

    // Close and unlink the semaphore
    sem_close(sem_key);
    sem_close(sem_action);


    return 0;

}

int read_key_from_pipe (int pipe_des)
{
    char msg[MSG_LEN];

    ssize_t bytes_read = read(pipe_des, msg, sizeof(msg));

    // printf("succesfully read from window\n");
    int pressed_key = msg[0];
    return pressed_key;
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &key_pressing[0], &km_server[1]);
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_action);

        printf("Succesfully closed all semaphores\n");
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


// US Keyboard assumed
char* determine_action(int pressed_key, char *shared_action)
{
    char key = toupper(pressed_key);
    int x; int y;
    // char action_msg[20];

    // Disclaimer: Y axis is inverted on tested terminal.
    if ( key == 'W')
    {
        x = 0;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // sprintf(action_msg, "%d, %d", x, y);
        // write_to_pipe(action_des[1], action_msg);
        // return "UP";
        return "0,-1";
    }
    if ( key == 'S')
    {
        x = 0;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "DOWN";
        return "0,1";
    }
    if ( key == 'A')
    {
        x = -1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "LEFT";
        return "-1,0";
    }
    if ( key == 'D')
    {
        x = 1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "RIGHT";
        return "1,0";
    }
    if ( key == 'Q')
    {
        x = -1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "UP-LEFT";
        return "-1,-1";
    }
    if ( key == 'E')
    {
        x = 1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "UP-RIGHT";
        return "1,-1";
    }
    if ( key == 'Z')
    {
        x = -1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "DOWN-LEFT";
        return "-1,1";
    }
    if ( key == 'C')
    {
        x = 1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(shared_action, "%d,%d", x, y);
        // return "DOWN-RIGHT";
        return "1,1";
    }
    if ( key == 'R')
    {
        x = 900;    // Special value interpreted by drone.c process
        y = 0;
        sprintf(shared_action, "%d,%d", x, y);
        // return "STOP";
        return "900,0";
    }
    else
    {
        return "None";
    }
}