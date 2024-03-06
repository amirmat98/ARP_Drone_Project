#include "drone.h"
#include "import.h"

int interface_drone[2];
int server_drone[2];
int drone_server[2];

char log_file[LOG_FILE_LEN];
char msg[MSG_BUF_LEN];


int main(int argc, char *argv[])
{
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Signals
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

    // Drone Vareables
    double position_x;
    double velocity_x = 0.0;    // Initial velocity of x
    double force_x = 0; // Applied force in the x direction
    double external_force_x = 0; // Initial external force

    double position_y;
    double velocity_y = 0.0;    // Initial velocity of y
    double force_y = 0; // Applied force in the y direction
    double external_force_y = 0; // Initial external force

    
    Obstacles obstacles[80];
    int number_obstacles;
    
    bool euler_method_flag = true; // For testing purposes.
    int valid_target;
    int obtained_obstacles = 0;

    //-----------------------------------------------------------------------------------------//


    // Simulate the motion in an infinite loop using Differential Equations
    while (1)
    {

        /* Step 1: READ DATA FROM SERVER */
          
        char server_msg[MSG_LEN];

        if (read_pipe_non_blocking(server_drone[0], server_msg) == 1)
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
                    sprintf(msg, "Obtained initial parameters from server: %s\n", server_msg);
                    log_msg(log_file, DRONE, msg);
                    position_x = (double)x;
                    position_y = (double)y;
                }
                // Message origin: Interface
                else if(server_msg[0] == 'I' && server_msg[1] == '2')
                {
                    sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                }
                // Message origin: Obstacles
                else if (server_msg[0 == 'O'])
                {
                    parse_obstacles_msg(server_msg, obstacles, &number_obstacles);
                    obtained_obstacles = 1;
                }
        }
        else
        {
            action_x = 0;
            action_y = 0;
        }

        // If obstacles has not been obtained, continue loop.
        if (obtained_obstacles == 0)
        {
            continue;
        }
   
        /* Step 2: READ DATA FROM INTERFACE.C */
        
        char interface_msg[MSG_LEN];

        if (read_pipe_non_blocking(interface_drone[0], interface_msg) == 1)
        {
            sscanf(interface_msg, "%d,%d", &target_x, &target_y);
            valid_target = 1;
        }

        
        /* Step 3: OBTAIN THE FORCES EXERTED ON THE DRONE*/
        
        if(euler_method_flag)
        {
            // Only values between -1 to 1 are used to move the drone
            if(action_x >= -1.0 && action_x <= 1.0)
            {
                force_x += (double)action_x;
                force_y += (double)action_y;
                /* Capping to the max value of the drone's force */
                boundrey_check_handler(&force_x, &force_y, F_MAX);
            }
            // Other values
            else
            {
                force_x = 0.0; 
                force_y = 0.0;
                if (action_x == 50.0 && action_y == 50.0)
                {
                    velocity_x = 0.0;
                    velocity_y = 0.0;
                }
            }

            /* EXTERNAL FORCE from obstacles and targets */
            double external_force_x = 0.0;
            double external_force_y = 0.0;

            // TARGETS
            if (valid_target == 1)
            {
                calculate_extenal_force(position_x, position_y, target_x, target_y, 0.0, 0.0, &external_force_x, &external_force_y);
            }

            // OBSTACLES
            for (int i = 0; i < number_obstacles; i++)
            {
                calculate_extenal_force(position_x, position_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &external_force_x, &external_force_y);
            }

            boundrey_check_handler(&external_force_x, &external_force_y, EXT_FORCE_MAX);

            
            /* Step 4: CALCULATE POSITION DATA */
            
            double max_x = (double)screen_size_x;
            double max_y = (double)screen_size_y;
            differential_equations(&position_x, &velocity_x, force_x, external_force_x, &max_x);
            differential_equations(&position_y, &velocity_y, force_y, external_force_y, &max_y);
            int x_f = (int)round(position_x);
            int y_f = (int)round(position_y);
            float v_x_f = (float)velocity_x;
            float v_y_f = (float)velocity_y;
      
            
            /* Step 5: SEND THE DRONE POSITION and VELOCITY TO SERVER */
            
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


void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y)
{
    // ***Calculate ATTRACTION force towards the target***
    double distance_to_target = sqrt(pow(target_x - drone_x, 2) + pow(target_y - drone_y, 2));
    double angle_to_target = atan2(target_y - drone_y, target_x - drone_x);

    // To avoid division by zero or extremely high forces
    if (distance_to_target < MAX_DISTANCE)
    {
        // Keep the force value as it were on the min_distance
        distance_to_target = MAX_DISTANCE;
    }
    else if (distance_to_target < MIN_DISTANCE)
    {
        *external_force_x += COEFFICIENT * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        *external_force_y += COEFFICIENT * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
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
    if (distance_to_obstacle < MAX_DISTANCE)
    {
        // Keep the force value as it were on the min_distance
        distance_to_obstacle = MAX_DISTANCE;
    }
    else if (distance_to_obstacle < MIN_DISTANCE)
    {
        *external_force_x -= COEFFICIENT * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * cos(angle_to_obstacle);
        *external_force_y -= COEFFICIENT * (1.0 / distance_to_obstacle - 1.0 / 5.0) * (1.0 / pow(distance_to_obstacle, 2)) * sin(angle_to_obstacle);
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
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);

        // Convert float to int (rounding is acceptable)
        obstacles[*number_obstacles].x = (int)(x_float + 0.5);
        obstacles[*number_obstacles].y = (int)(y_float + 0.5);

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
        sprintf(msg, "Caught SIGINT \n");
        log_msg(log_file, DRONE, msg);
        close(drone_server[0]);
        close(drone_server[1]);
        close(interface_drone[0]);
        close(interface_drone[1]);
        close(server_drone[0]);
        close(server_drone[1]);
        sprintf(msg,"Succesfully performed the cleanup\n");
        log_msg(log_file, DRONE, msg);
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
    sscanf(argv[1], "%d %d %d %s", &server_drone[0], &drone_server[1], &interface_drone[0], log_file);
}

void boundrey_check_handler(double *x, double *y, double condition)
{
    if (*x > condition)
    {
        *x = condition;
    }
    if (*x < -condition)
    {
        *x = -condition;
    }
    if (*y > condition)
    {
        *y = condition;
    }
    if (*y < -condition)
    {
        *y = -condition;
    }
}