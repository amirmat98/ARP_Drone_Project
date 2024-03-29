#include "drone.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>       
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// Serverless pipes
int lowest_target_fd[2];

// Pipes working with the server
int server_drone[2];
int drone_server[2];


int main(int argc, char *argv[])
{
    get_args(argc, argv);

    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    
    publish_pid_to_wd(DRONE_SYM, getpid());

    //-----------------------------------------------------------------------------------------//
    // Variable declaration
    int x; int y;
    int screen_size_x; int screen_size_y;
    int action_x; int action_y;    
    int target_x; int target_y;
    int valid_target;


    // Variables for differential_equations
    double pos_x;
    double v_x = 0.0;    // Initial velocity of x
    double force_x = 0; // Applied force in the x direction
    double external_force_x = 0; // Initial external force

    double pos_y;
    double v_y = 0.0;    // Initial velocity of y
    double force_y = 0; // Applied force in the y direction
    double external_force_y = 0; // Initial external force

    int obtained_obstacles = 0;
    Obstacles obstacles[80];
    int number_obstacles;
    
    bool euler_method_flag = true; // For testing purposes.

    char server_msg[MSG_LEN];

    // Simulate the motion in an infinite loop using Differential Equations
    while (1)
    {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ DATA FROM SERVER */
        /////////////////////////////////////////////////////
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_drone[0], &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

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
                 // Message origin: Keyboard Manager
                if(server_msg[0] == 'K')
                {
                    sscanf(server_msg, "K:%d,%d", &action_x, &action_y);
                }
                // Message origin: Interface
                else if(server_msg[0] == 'I' && server_msg[1] == '1')
                {
                    sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y);
                    printf("Obtained initial parameters from server: %s\n", server_msg);
                    pos_x = (double)x;
                    pos_y = (double)y;
                    fflush(stdout);
                }
                // Message origin: Interface
                else if(server_msg[0] == 'I' && server_msg[1] == '2')
                {
                    sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                    // printf("Changed screen dimensions to: %s\n", server_msg);
                    fflush(stdout);
                }
                // Message origin: Obstacles
                else if (server_msg[0 == 'O'])
                {
                    // printf("Obtained obstacles message: %s\n", server_msg);
                    parse_obstacles_msg(server_msg, obstacles, &number_obstacles);
                    obtained_obstacles = 1;
                }
            }
        }
        else
        {
            action_x = 0;
            action_y = 0;
        }


        char obstacles_msg[] = "O[1]200,200"; // Random for initial execution
        if (obtained_obstacles == 0)
        {
            parse_obstacles_msg(obstacles_msg, obstacles, &number_obstacles);
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ DATA FROM INTERFACE.C (SERVERLESS PIPE) */
        /////////////////////////////////////////////////////

        fd_set read_interface;
        FD_ZERO(&read_interface);
        FD_SET(lowest_target_fd[0], &read_interface);
        
        char msg[MSG_LEN];

        int ready_targets = select(lowest_target_fd[0] + 1, &read_interface, NULL, NULL, &timeout);
        if (ready_targets == -1) 
        {
            perror("Error in select");
        }

        if (ready_targets > 0 && FD_ISSET(lowest_target_fd[0], &read_interface))
        {
            ssize_t bytes_read_interface = read(lowest_target_fd[0], msg, MSG_LEN);
            if (bytes_read_interface > 0) 
            {
                // Read acknowledgement
                // printf("RECEIVED %s from interface.c\n", msg);
                sscanf(msg, "%d,%d", &target_x, &target_y);
                fflush(stdout);
                valid_target = 1;
            }
            else if (bytes_read_interface == -1) 
            {
                perror("Read pipe lowest_target_fd");
            }
        }

        //////////////////////////////////////////////////////
        /* SECTION 3: OBTAIN THE FORCES EXERTED ON THE DRONE*/
        /////////////////////////////////////////////////////  

        if(euler_method_flag)
        {
            /* INTERNAL FORCE from the drone itself */
            // Only values between -1 to 1 are used to move the drone
            if(action_x >= -1.0 && action_x <= 1.0)
            {
                force_x += (double)action_x;
                force_y += (double)action_y;
                /* Capping to the max value of the drone's force */
                if(force_x > F_MAX)
                    {force_x = F_MAX;}
                if(force_y > F_MAX)
                    {force_y = F_MAX;}
                if(force_x < -F_MAX)
                    {force_x = -F_MAX;}
                if(force_y < -F_MAX)
                    {force_y = -F_MAX;}
            }
            // Other values represent STOP
            else
            {
                force_x = 0.0; 
                force_y = 0.0;
            }

            /* EXTERNAL FORCE from obstacles and targets */
            double external_force_x = 0.0;
            double external_force_y = 0.0;

            // TARGETS
            if (valid_target == 1)
            {
                calculate_extenal_force(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &external_force_x, &external_force_y);
            }


            // OBSTACLES
            for (int i = 0; i < number_obstacles; i++)
            {
                calculate_extenal_force(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &external_force_x, &external_force_y);
            }

            if(external_force_x > EXT_FORCE_MAX)
            {
                external_force_x = 0.0;
            }
            if(external_force_x < -EXT_FORCE_MAX)
            {
                external_force_x = 0.0;
            }
            if(external_force_y > EXT_FORCE_MAX)
            {
                external_force_y = 0.0;
            }
            if(external_force_y < -EXT_FORCE_MAX)
            {
                external_force_y = 0.0;
            }

            //////////////////////////////////////////////////////
            /* SECTION 4: CALCULATE POSITION DATA */
            ///////////////////////////////////////////////////// 

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

            
            //////////////////////////////////////////////////////
            /* SECTION 5: SEND THE DRONE POSITION TO SERVER */
            ///////////////////////////////////////////////////// 
            char position_msg[MSG_LEN];
            sprintf(position_msg, "D:%d,%d", x_f, y_f);
            write_to_pipe(drone_server[1], position_msg);
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
        *external_force_x += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        *external_force_y += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
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
        *external_force_x -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * cos(angle_to_obstacle);
        *external_force_y -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * sin(angle_to_obstacle);
    }
    else
    {
        *external_force_x -= 0;
        *external_force_y -= 0;
    }

}

void parse_obstacles_msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles)
{
    int total_obstacles;
    sscanf(obstacles_msg, "O[%d]", &total_obstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *number_obstacles = 0;

    while (token != NULL && *number_obstacles < total_obstacles)
    {
        sscanf(token, "%d,%d", &obstacles[*number_obstacles].x, &obstacles[*number_obstacles].y);
        obstacles[*number_obstacles].total = *number_obstacles + 1;

        token = strtok(NULL, "|");
        (*number_obstacles)++;
    }
}

// Watchdog Function
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
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
    sscanf(argv[1], "%d %d %d", &server_drone[0], &drone_server[1], &lowest_target_fd[0]);
}