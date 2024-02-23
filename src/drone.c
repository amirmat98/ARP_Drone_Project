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
    
    Target low_index_target;
    bool valid_target;

    Drone drone;
    drone.velocity_x = 0.0;
    drone.velocity_y = 0.0;
    drone.force_x = 0.0;
    drone.force_y = 0.0;

    bool obtained_obstacles = 0;
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
        
        // Initialize a file descriptor set
        fd_set read_set;
        FD_ZERO(&read_set); // Clear all entries from the set
        FD_SET(server_drone[0], &read_set); // Add the server_drone's read-end file descriptor to the set

        // Set up a timeout structure
        struct timeval timeout;
        timeout.tv_sec = 0; // Set the timeout to 0 seconds
        timeout.tv_usec = 0; // ...and 0 microseconds


        // Perform a select call to see if there's data ready to be read
        int ready = select (server_drone[0] + 1, &read_set, NULL, NULL, &timeout);
        if (ready == -1)
        {
            // If select returns -1, an error has occurred
            perror("Error in select");
        }

        // If select returns a value greater than 0, then there is data to be read
        if (ready > 0 && FD_ISSET(server_drone[0], &read_set))
        {
            ssize_t bytes_read = read(server_drone[0], server_msg, MSG_LEN);
            if (bytes_read > 0)
            {
                switch (server_msg[0])
                {
                    case 'K': // Message origin: Keyboard Manager
                        sscanf(server_msg, "K:%d,%d", &action_x, &action_y);
                        break;

                    case 'I': // Message origin: Map
                        switch (server_msg[1])
                        {
                            case '1': // Message format I1
                                sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y);
                                printf("Obtained initial parameters from server: %s\n", server_msg);
                                drone.position_x = (double)x;
                                drone.position_y = (double)y;
                                fflush(stdout);
                                break;
                            case '2': // Message format I2
                                sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                                printf("Changed screen dimensions to: %s\n", server_msg);
                                fflush(stdout);
                                break;
                            default:
                                break;
                        }
                    
                    case 'O': // Message origin: Obstacle
                        // Assuming there's a typo in your code and fixing it to server_msg[0] == 'O'
                        printf("Obtained obstacles message: %s\n", server_msg);
                        parse_obstacles_msg(server_msg, obstacles, &number_obstacles);
                        obtained_obstacles = true;
                        break;

                    default:
                        break;
                }
            }
        }
        else
        {
            // If there's no data ready, reset the action coordinates
            action_x = 0;
            action_y = 0;
        }

        // Set up a message with obstacle data for initial execution
        char obstacles_msg[] = "O[1]125,175"; // Random for initial execution
        // If we haven't yet obtained obstacle information, parse the initial obstacle message
        if (!obtained_obstacles)
        {
            parse_obstacles_msg(obstacles_msg, obstacles, &number_obstacles);
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ DATA FROM INTERFACE.C (SERVERLESS PIPE) */
        /////////////////////////////////////////////////////

        fd_set read_map;
        FD_ZERO(&read_map);
        FD_SET(lowest_target_fd[0], &read_map);
        
        char msg[MSG_LEN];

        int ready_targets = select(lowest_target_fd[0] + 1, &read_map, NULL, NULL, &timeout);
        if (ready_targets == -1) 
        {
            perror("Error in select");
        }

        if (ready_targets > 0 && FD_ISSET(lowest_target_fd[0], &read_map))
        {
            ssize_t bytes_read_interface = read(lowest_target_fd[0], msg, MSG_LEN);
            if (bytes_read_interface > 0) 
            {
                // Read acknowledgement
                // printf("RECEIVED %s from interface.c\n", msg);
                sscanf(msg, "%d,%d", &low_index_target.x, &low_index_target.y);
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

            /* INTERNAL FORCE from the drone itself */
            // Only values between -1 to 1 are used to move the drone
            if(action_x >= -1.0 && action_x <= 1.0)
            {
                drone.force_x += (double)action_x;
                drone.force_y += (double)action_y;
                /* Capping to the max value of the drone's force */
                if(drone.force_x > F_MAX)
                    {drone.force_x = F_MAX;}
                if(drone.force_y > F_MAX)
                    {drone.force_y = F_MAX;}
                if(drone.force_x < -F_MAX)
                    {drone.force_x = -F_MAX;}
                if(drone.force_y < -F_MAX)
                    {drone.force_y = -F_MAX;}
            }
            // Other values represent STOP
            else
            {
                drone.force_x = 0.0; 
                drone.force_y = 0.0;
            }

            /* EXTERNAL FORCE from obstacles and targets */
            drone.external_force_x = 0.0;
            drone.external_force_y = 0.0;

            // TARGETS
            if (valid_target == 1)
            {
                Obstacles temp_obs;
                temp_obs.x = 0;
                temp_obs.y = 0;
                calculate_extenal_force(&drone, &low_index_target, &temp_obs);
                // calculate_extenal_force(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &external_force_x, &external_force_y);
            }


            // OBSTACLES
            Target temp_targ;
            temp_targ.x = 0;
            temp_targ.y = 0;
            for (int i = 0; i < number_obstacles; i++)
            {
                calculate_extenal_force(&drone, &temp_targ, &obstacles[i]);
                // calculate_extenal_force(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &external_force_x, &external_force_y);
            }

            if(drone.external_force_x > EXT_FORCE_MAX)
            {
                drone.external_force_x = 0.0;
            }
            if(drone.external_force_x < -EXT_FORCE_MAX)
            {
                drone.external_force_x = 0.0;
            }
            if(drone.external_force_y > EXT_FORCE_MAX)
            {
                drone.external_force_y = 0.0;
            }
            if(drone.external_force_y < -EXT_FORCE_MAX)
            {
                drone.external_force_y = 0.0;
            }

            //////////////////////////////////////////////////////
            /* SECTION 4: CALCULATE POSITION DATA */
            ///////////////////////////////////////////////////// 

            differential_equations (&drone, screen_size_x, screen_size_y);

            // Only print the positions when there is still velocity present.
            if(fabs(drone.velocity_x) > FLOAT_TOLERANCE || fabs(drone.velocity_y) > FLOAT_TOLERANCE)
            {
                printf("Drone Force (X,Y): %.2f,%.2f\t|\t",drone.force_x, drone.force_y);
                printf("External Force (X,Y): %.2f,%.2f\n",drone.external_force_x, drone.external_force_y);
                printf("X - Position: %.2f / Velocity: %.2f\t|\t", drone.position_x, drone.velocity_x);
                printf("Y - Position: %.2f / Velocity: %.2f\n", drone.position_y, drone.velocity_y);
                fflush(stdout);
            }
            int x_f = (int)round(drone.position_x);
            int y_f = (int)round(drone.position_y);

            
            //////////////////////////////////////////////////////
            /* SECTION 5: SEND THE DRONE POSITION TO SERVER */
            ///////////////////////////////////////////////////// 
            char position_msg[MSG_LEN];
            sprintf(position_msg, "D:%d,%d", x_f, y_f);
            write_to_pipe(drone_server[1], position_msg);

        // Introduce a delay on the loop to simulate real-time intervals.
        usleep(D_T * 1e6); 
    }

    return 0; 
}


// Implementation of the Differential Equations function
void differential_equations(Drone *drone, double screen_x, double screen_y)
{
    double max_x = (double)screen_x;
    double max_y = (double)screen_y;

    double total_force_x = drone->force_x + drone->external_force_x;
    double total_force_y = drone->force_y + drone->external_force_y;

    double acceleration_x = (total_force_x - DAMPING * (drone->velocity_x)) / MASS;
    double acceleration_y = (total_force_y - DAMPING * (drone->velocity_y)) / MASS;

    drone->velocity_x = drone->velocity_x + acceleration_x * D_T;
    drone->velocity_y = drone->velocity_y + acceleration_y * D_T;

    drone->position_x = drone->position_x + drone->velocity_x * D_T;
    drone->position_y = drone->position_y + drone->velocity_y * D_T;

    // Walls are the maximum position the drone can reach

    if(drone->position_x < 0.0)
    {
        drone->position_x = 0.0;
    }
    if(drone->position_x > max_x)
    {
        drone->position_x = max_x - 1;
    }

    if (drone->position_y < 0.0)
    {
        drone->position_y = 0.0;
    }

    if(drone->position_y > max_y)
    {
        drone->position_y = max_y - 1;
    }

}

// Moving the drone step by step as initial development
void step_method(int *x, int *y, int action_x, int action_y)
{
    (*x) = (*x) + action_x;
    (*y) = (*y) + action_y;


    //sprintf(shared_action, "%d,%d", 0, 0); // Zeros written on action memory
}


void calculate_extenal_force(Drone *drone, Target *target, Obstacles *obstacle)
{
    // ***Calculate ATTRACTION force towards the target***
    double distance_to_target = sqrt(pow(target->x - drone->position_x, 2) + pow(target->y - drone->position_y, 2));
    // double distance_to_target = sqrt(pow(target_x - drone_x, 2) + pow(target_y - drone_y, 2));
    double angle_to_target = atan2(target->y - drone->position_y, target->x - drone->position_x);
    // double angle_to_target = atan2(target_y - drone_y, target_x - drone_x);

    // To avoid division by zero or extremely high forces
    if (distance_to_target < min_distance)
    {
        // Keep the force value as it were on the min_distance
        distance_to_target = min_distance;
    }
    else if (distance_to_target < start_distance)
    {
        drone->external_force_x += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        // *external_force_x += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        drone->external_force_y += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
        // *external_force_y += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
    }
    else
    {
        drone->external_force_x += 0;
        drone->external_force_y += 0;
    }
    
    // ***Calculate REPULSION force from the obstacle***
    double distance_to_obstacle = sqrt(pow(obstacle->x - drone->position_x, 2) + pow(obstacle->y - drone->position_y, 2));
    // double distance_to_obstacle = sqrt(pow(obstacle_x - drone_x, 2) + pow(obstacle_y - drone_y, 2));
    double angle_to_obstacle = atan2(obstacle->y - drone->position_y, obstacle->x - drone->position_x);
    // double angle_to_obstacle = atan2(obstacle_y - drone_y, obstacle_x - drone_x);

    // To avoid division by zero or extremely high forces
    if (distance_to_obstacle < min_distance)
    {
        // Keep the force value as it were on the min_distance
        distance_to_obstacle = min_distance;
    }
    else if (distance_to_obstacle < start_distance)
    {
        drone->external_force_x -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * cos(angle_to_obstacle);
        // *external_force_x -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * cos(angle_to_obstacle);
        drone->external_force_y -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * sin(angle_to_obstacle);
        // *external_force_y -= Coefficient * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * sin(angle_to_obstacle);
    }
    else
    {
        drone->external_force_x -= 0;
        drone->external_force_y -= 0;
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