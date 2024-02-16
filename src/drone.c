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
#include <sys/select.h>
#include <sys/stat.h>

#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// Pipes working with the server
int server_drone[2];
int drone_server[2];


int main(int argc, char *argv[])
{
    get_args(argc, argv);

    // Watchdog Variables
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    
    publish_pid_to_wd(DRONE_SYM, getpid());

    usleep(SLEEP_DRONE); // To let the interface.c process execute first and write the initial positions.


    //-----------------------------------------------------------------------------------------//
    // Variable declaration
    int x; int y;
    int screen_size_x; int screen_size_y;
    int action_x; int action_y;


    // Variables for differential_equations
    double pos_x;
    double v_x = 0.0;    // Initial velocity of x
    double force_x = 0; // Applied force in the x direction
    double external_force_x = 0; // Initial external force

    double pos_y;
    double v_y = 0.0;    // Initial velocity of y
    double force_y = 0; // Applied force in the y direction
    double external_force_y = 0; // Initial external force
    
    bool euler_method_flag = true; // For testing purposes.

    char server_msg[MSG_LEN];

    // Simulate the motion in an infinite loop using Differential Equations
    while (1)
    {

        /*-----------------------------------------------------------*/
        /* READ the pipe from SERVER*/
        
        fd_set = read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_drone[0], &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        /*-------------------------------------------------------------*/

        int ready = select (server_drone[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1)
        {
            perror("Error in select");
        }

        if (ready > 0 && FD_ISSET(server_drone[0], &read_fds))
        {
            ssize_t bytes_read = read(server_drone[0], server_msg, MSG_LEN);
            if (bytes_read > 0)
            {
                if(decypher_message(server_msg) == 1)
                {
                    sscanf(server_msg, "K:%d,%d", &action_x, &action_y);
                }
                else if(decypher_message(server_msg) == 2)
                {
                    sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y);
                    printf("Obtained initial parameters from server: %s\n", server_msg);
                    pos_x = (double)x;
                    pos_y = (double)y;
                    fflush(stdout);
                }
                else if(decypher_message(server_msg) == 3)
                {
                    sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                    printf("Changed screen dimensions to: %s\n", server_msg);
                    fflush(stdout);
                }
            }
        }
        else
        {
            action_x = 0;
            action_y = 0;
        }

        /*-----------------------------------------------------------*/


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

            // Calculate the EXTERNAL FORCE from obstacles and targets

            // TARGETS
            char targets_msg[] = "140,23";
            double target_x, target_y;
            sscanf(targets_msg, "%1f,%1f", &target_x, &target_y);
            calculate_extenal_force(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &external_force_x, &external_force_y);


            // OBSTACLES
            char obstacles_msg[] = "O[7]35,11|100,5|16,30|88,7|130,40|53,15|60,10";
            Obstacles obstacles[30];
            int number_obstacles;
            parse_obstacles_Msg(obstacles_msg, obstacles, &number_obstacles);

            for (int i = 0; i < number_obstacles; i++)
            {
                calculate_extenal_force(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &external_force_x, &external_force_y);
            }

            // Calling the function
            double max_x = (double)screen_size_x;
            double max_y = (double)screen_size_y;
            differential_equations(&pos_x, &v_x, force_x, external_force_x, &max_x);
            differential_equations(&pos_y, &v_y, force_y, external_force_y, &max_y);

            // Only print the positions when there is still velocity present.
            if(fabs(v_x) > FLOAT_TOLERANCE || fabs(v_y) > FLOAT_TOLERANCE)
            {
                printf("Drone Force (X,Y): %.2f,%.2f\t|\t",force_x,force_y);
                printf("External Force (X,Y): %.2f,%.2f\n",external_force_x,external_force_y);
                printf("X - Position: %.2f / Velocity: %.2f\t|\t", pos_x, v_x);
                printf("Y - Position: %.2f / Velocity: %.2f\n", pos_y, v_y);
                fflush(stdout);
            }
            int x_f = (int)round(pos_x);
            int y_f = (int)round(pos_y);

            // Using pipes write this data so it can be read from (interface.c)
            char position_msg[MSG_LEN];
            sprintf(position_msg, "D:%d,%d", x_f, y_f);
            write_to_pipe(drone_server[1], position_msg);
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

    return 0; 
}


// Implementation of the Differential Equations function
void differential_equations(double *position, double *velocity, double force, double external_force, double *max_pos)
{
    double total_force = force + external_force;
    double acceleration_x = (total_force - DAMPING * (*velocity)) / MASS;
    *velocity = *velocity + acceleration_x * D_T;
    *position = *position + (*velocity) * D_T;

    // Walls are the maximum position the drone can reach
    if (*position < 0) { *position = 0; }
    if (*position > *max_pos) { *position = *max_pos - 1; }

}

// Moving the drone step by step as initial development
void step_method(int *x, int *y, int action_x, int action_y)
{
    (*x) = (*x) + action_x;
    (*y) = (*y) + action_y;


    //sprintf(shared_action, "%d,%d", 0, 0); // Zeros written on action memory
}


void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y)
{
    // ***Calculate ATTRACTION force towards the target***
    double distance_to_target = sqrt(pow(target_x - drone_x, 2) + pow(target_y - drone_y, 2));
    double angle_to_target = atan2(target_y - drone_y, target_x - drone_x);

    // To avoid division by zero or extremely high forces
    if (distance_to_target < min_distance)
    {
        // Keep the force value as it were on the min_distance
        distance_to_target = min_distance;
    }
    else if (distance_to_target < start_distance)
    {
        *external_force_x += coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        *external_force_y += coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
    }
    else
    {
        *external_force_x += 0;
        *external_force_y += 0;
    }
    
    // ***Calculate REPULSION force from the obstacle***
    double distance_to_obstacle = sqrt(pow(obstacle_x - drone_x, 2) + pow(obstacle_y - drone_y, 2));
    double angle_to_obstacle = atan2(obstacle_y - drone_y, obstacle_x - drone_x);

    // To avoid division by zero or extremely high forces
    if (distance_to_obstacle < min_distance)
    {
        // Keep the force value as it were on the min_distance
        distance_to_obstacle = min_distance;
    }
    else if (distance_to_obstacle < start_distance)
    {
        external_force_x -= coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * cos(angle_to_obstacle);
        external_force_y -= coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * sin(angle_to_obstacle);
    }
    else
    {
        *external_force_x -= 0;
        *external_force_y -= 0;
    }

    /*
    // TO FIX A BUG WITH BIG FORCES APPEARING OUT OF NOWHERE
    if (*external_force_x > 50) {*external_force_x = 0;}
    if (*external_force_y > 50) {*external_force_y = 0;}
    if (*external_force_x < 50) {*external_force_x = 0;}
    if (*external_force_y < 50) {*external_force_y = 0;}
    */


}

void parse_obstacles_Msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles)
{
    int total_obstacles;
    sscanf(obstacles_msg, "0[%d]", &total_obstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *number_obstacles = 0;

    while (token != NULL && *number_obstacles < total_obstacles)
    {
        sscanf(token, "%d , %d", &obstacles[*number_obstacles].x, &obstacles[*number_obstacles].y);
        obstacles[*number_obstacles].total = *number_obstacles + 1;

        token = strtok(NULL, "|");
        (*number_obstacles)++;
    }
}

// Watchdog Function
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    printf("Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        // sem_close(sem_action);

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

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_drone[0], &drone_server[1]);
}

int decypher_message(const char *server_msg) 
{
    if (server_msg[0] == 'K') 
    {
        return 1;
    } 
    else if (server_msg[0] == 'I' && server_msg[1] == '1') 
    {
        return 2;
    } 
    else if (server_msg[0] == 'I' && server_msg[1] == '2') 
    {
        return 3;
    }
    else 
    {
        return 0;
    }
}