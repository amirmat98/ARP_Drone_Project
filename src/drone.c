#include "drone.h"
#include "constants.h"
#include "util.h"
#include <stdio.h>       
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>

/* Global variables */
// Initialize shared memory for drone positions
int shared_pos; // File descriptor for drone position shm
char *shared_position; // Shared memory for Drone Position
// Initialize shared memory for drone actions.
int shared_act; // File descriptor for actions shm
char *shared_action; // Shared memory for drone action 
// Initialize semaphores
sem_t *sem_pos; // semaphore for drone positions
sem_t *sem_action; // semaphore for drone action


// Watchdog Function
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    printf("Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
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

int main()
{
    // Watchdog Variables
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    
    publish_pid_to_wd(DRONE_SYM, getpid());

    // Shared memory for DRONE POSITION
    shared_pos = shm_open(SHAREMEMORY_POSITION, O_RDWR, 0666);
    shared_position = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shared_pos, 0);
    sem_pos = sem_open(SEMAPHORE_POSITION, 0);

    // Shared memory for DRONE CONTROL - ACTION
    shared_act = shm_open(SHAREMEMORY_ACTION, O_RDWR, 0666);
    shared_action = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shared_act, 0);
    sem_action = sem_open(SEMAPHORE_ACTION, 0);


    //-----------------------------------------------------------------------------------------//
    // Variable declaration
    usleep(SLEEP_DRONE);
    int x; int y;
    int max_x; int max_y;
    int action_x; int action_y;
    sscanf(shared_position, "%d,%d,%d,%d", &x, &y, &max_x, &max_y); // Obtain the values of X,Y from shared memory


    // Variables for differential_equations
    double pos_x = (double)x;
    double v_x = 0.0;    // Initial velocity of x
    double force_x = 0; // Applied force in the x direction

    double pos_y = (double)y;
    double v_y = 0.0;    // Initial velocity of y
    double force_y = 0; // Applied force in the y direction
    
    bool euler_method_flag = true; // For testing purposes.

    // Simulate the motion in an infinite loop using Differential Equations
    while (1)
    {
        int x_i; int y_i;
        sscanf(shared_position, "%d,%d,%d,%d", &x_i, &y_i, &max_x, &max_y);
        sscanf(shared_action, "%d,%d", &action_x, &action_y);

        /* DRONE CONTROL WITH THE DYNAMICS FORMULA*/
        if(euler_method_flag)
        {
            // Only values between -1 to 1 are used to move the drone
            if(action_x >= -1.0 && action_x <= 1.0)
            {
                force_x += (double)action_x;
                force_y += (double)action_y;
                // Capping the force to a maximum value
                if(force_x>F_MAX)
                    {force_x = F_MAX;}
                if(force_y>F_MAX)
                    {force_y = F_MAX;}
                if(force_x<-F_MAX)
                    {force_x = -F_MAX;}
                if(force_y<-F_MAX)
                    {force_y = -F_MAX;}
            }
            // For now, other values for action represent a STOP command.
            else
            {
                force_x = 0.0; 
                force_y = 0.0;
            }

            // Calling the function
            double max_x_f = (double)max_x;
            double max_y_f = (double)max_y;
            differential_equations(&pos_x, &v_x, force_x, &max_x_f);
            differential_equations(&pos_y, &v_y, force_y, &max_y_f);

            // Only print the positions when there is still velocity present.
            if(fabs(v_x) > FLOAT_TOLERANCE || fabs(v_y) > FLOAT_TOLERANCE)
            {
                printf("Force (X,Y): %.2f,%.2f\n",force_x,force_y);
                printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, v_x);
                printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, v_y);
                fflush(stdout);
            }
            // Write zeros on action memory
            sprintf(shared_action, "%d,%d", 0, 0);
            // Write new drone position to shared memory
            int x_f = (int)round(pos_x);
            int y_f = (int)round(pos_y);
            sprintf(shared_position, "%d,%d,%d,%d", x_f, y_f, max_x, max_y);
            sem_post(sem_pos);
        }

        /* DRONE CONTROL WITH THE STEP METHOD*/
        else
        {
            if(action_x >= -1.0 && action_x <= 1.0)
            {
                // Calling the function
                step_method(&x,&y,action_x,action_y);
                // Only print when there is change in the position.
                if(action_x!=0 || action_y!=0)
                {
                    printf("Action (X,Y): %s\n",shared_action);
                    printf("X - Position: %d / Velocity: %.2f\t|\t", x, v_x);
                    printf("Y - Position: %d / Velocity: %.2f\n", y, v_y);
                    fflush(stdout);
                }
                sprintf(shared_action, "%d,%d", 0, 0); // Zeros written on action memory
                // Write new drone position to shared memory
                sprintf(shared_position, "%d,%d,%d,%d", x, y, max_x, max_y);
                sem_post(sem_pos);
            }
        }
        // Introduce a delay on the loop to simulate real-time intervals.
        usleep(D_T * 1e6); 
    }
    // Detach the shared memory segment
    close(shared_pos);
    close(shared_act);
    sem_close(sem_pos);
    sem_close(sem_action);

    return 0; 
}


// Implementation of the Differential Equations function
void differential_equations(double *position, double *velocity, double force, double *max_pos)
{
    double acceleration_x = (force - DAMPING * (*velocity)) / MASS;
    *velocity = *velocity + acceleration_x * D_T;
    *position = *position + (*velocity) * D_T;

    if (*position < 0) { *position = 0; }
    if (*position > *max_pos) { *position = *max_pos - 1; }

}