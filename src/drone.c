#include "drone.h"
#include "import.h"

// ---------------------------------------------------------------- //
// Define the pipes
int lowest_target_fd[2];
int server_drone[2];
int drone_server[2];

int main(int argc, char *argv[])
{
    get_args(argc, argv); // Get the command line arguments

    // Signal handling
    struct sigaction sa; // Structure for the signal action
    sa.sa_sigaction = signal_handler; // Function to handle the signal
    sa.sa_flags = SA_SIGINFO; // Signal action flags
    sigaction(SIGINT, &sa, NULL); // Signal handler
    sigaction (SIGUSR1, &sa, NULL); // Signal handler
    publish_pid_to_wd(DRONE_SYM, getpid()); // Publish the PID to the Watchdog

    //-----------------------------------------------------------------------------------------//
    // Variable declaration
    int x; int y; // X and Y coordinates of the drone
    int screen_size_x; int screen_size_y; // X and Y coordinates of the screen
    int action_x = 0; int action_y = 0; // X and Y coordinates of the drone's action
    int target_x; int target_y; // X and Y coordinates of the drone's target
    int valid_target; // Flag to check if the drone's target is valid or not

    // Variables for differential_equations
    double pos_x; // Position of the drone in the x direction
    double v_x = 0.0;    // Initial velocity of x
    double force_x = 0; // Applied force in the x direction
    double external_force_x = 0; // Initial external force

    double pos_y; // Position of the drone in the y direction
    double v_y = 0.0;    // Initial velocity of y
    double force_y = 0; // Applied force in the y direction
    double external_force_y = 0; // Initial external force

    int obtained_obstacles = 0; // Flag to check if the drone has obtained an obstacle or not
    Obstacles obstacles[80]; // Used to store the obstacles
    int number_obstacles; // Number of the obstacles
    
    bool euler_method_flag = true; // For testing purposes.

    char server_msg[MSG_LEN]; // Message to be sent to the server

    // Simulate the motion in an infinite loop using Differential Equations
    while (1)
    {
        // -------------------------------------------------------------------- //
        /* Step 1: READ DATA FROM SERVER */
        
        // Read from the server
        if (read_from_pipe(server_drone[0], server_msg)) // Read from the server pipe if there is a message to be read from the server
        {
            switch (server_msg[0]) // Switch on the first character of the message
            {
            case 'K': // Keyboard Manager message 
                sscanf(server_msg, "K:%d,%d", &action_x, &action_y); // Read the keyboard manager's action
                printf("Received keyboard manager's action: %d, %d\n", action_x, action_y); // Print the action
                break;
            case 'I':
                // Interface Manager message 
                if (server_msg[1] == '1') // if the message is I1 means that the initial value of screen_size_x and screen_size_y has been received
                {
                    sscanf(server_msg, "I1:%d,%d,%d,%d", &x, &y, &screen_size_x, &screen_size_y); // Read the interface manager's initial parameters
                    printf("Obtained initial parameters from server: %s\n", server_msg); // Print the initial parameters
                    pos_x = (double)x; // Set the position of the drone in the x direction
                    pos_y = (double)y; // Set the position of the drone in the y direction
                    fflush(stdout); // Flush the stdout buffer
                }
                else if (server_msg[1] == '2') // if the message is I2 means that the screen's size has been changed
                {
                    sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y); // Read the interface manager's screen dimensions
                    printf("Changed screen dimensions to: %s\n", server_msg); // Print the screen dimensions
                    fflush(stdout); // Flush the stdout buffer
                }
            case 'O':
                // Obstacles Manager message
                printf("Obtained obstacles message: %s\n", server_msg); // Print the obstacles message
                parse_obstacles_msg(server_msg, obstacles, &number_obstacles); // Parse the obstacles message and store the obstacles in the obstacles array
                obtained_obstacles = 1; // Set the obtained_obstacles flag to 1
            default:
                action_x = 0; // Set the action to 0
                action_y = 0; // Set the action to 0
                break;
            }
        }

        char obstacles_msg[] = "O[1]200,200"; // Random for initial execution
        if (obtained_obstacles == 0) // If the drone has not obtained an obstacle
        {
            parse_obstacles_msg(obstacles_msg, obstacles, &number_obstacles); // Parse the obstacles message and store the obstacles in the obstacles array
        }

        // -------------------------------------------------------------------- //
        /* Step 2: READ DATA FROM INTERFACE.C */
    
        char msg[MSG_LEN]; // Message is received from the interface
        if (read_from_pipe(lowest_target_fd[0], msg)) // Read from the lowest target pipe if there is a message to be read from the lowest target
        {
            // Read acknowledgement
            printf("RECEIVED %s from interface.c\n", msg); // Print the acknowledgement
            sscanf(msg, "%d,%d", &target_x, &target_y); // Read the coordinates of the lowest target
            fflush(stdout);
            valid_target = 1;
        }

        // -------------------------------------------------------------------- //
        /* Step 3: OBTAIN THE FORCES EXERTED ON THE DRONE*/ 

        if(euler_method_flag)
        {
            // Only values between -1 to 1 are used to move the drone
            if(action_x >= -1.0 && action_x <= 1.0) // check the value of the action
            {
                force_x += (double)action_x; // Add the action to the force in the x direction
                force_y += (double)action_y; // Add the action to the force in the y direction
                force_check_handler(&force_x, &force_y, F_MAX, F_MAX); // Check if the force is valid
            }
            // Other values represent STOP
            else
            {
                force_x = 0.0; // if gets the action to stop the drone, set the force to 0
                force_y = 0.0; // if gets the action to stop the drone, set the force to 0
                if (action_x == 50.0 && action_y == 50.0) // if gets the action to stop the drone in the emergency situation, set the velocity to 0 additionally
                {
                    v_x = 0.0; // set the velocity to 0 in the x direction
                    v_y = 0.0; // set the velocity to 0 in the y direction
                }
            }

            /* EXTERNAL FORCE from obstacles and targets */
            double external_force_x = 0.0; // Initial external force in the x direction
            double external_force_y = 0.0; // Initial external force in the y direction

            // TARGETS
            if (valid_target == 1) // If the drone's target is valid
            {
                // Calculate the external force on the drone's target
                calculate_extenal_force(pos_x, pos_y, target_x, target_y, 0.0, 0.0, &external_force_x, &external_force_y); 
            }

            // OBSTACLES
            for (int i = 0; i < number_obstacles; i++)
            {
                // Calculate the external force on the drone's obstacle
                calculate_extenal_force(pos_x, pos_y, 0.0, 0.0, obstacles[i].x, obstacles[i].y, &external_force_x, &external_force_y);
            }

            force_check_handler(&external_force_x, &external_force_y, EXT_FORCE_MAX, 0.0); // Check if the external force is valid

            // -------------------------------------------------------------------- //
            /* Step 4: CALCULATE POSITION DATA */

            double max_x = (double)screen_size_x; // Maximum value of the drone's position in the x direction
            double max_y = (double)screen_size_y; // Maximum value of the drone's position in the y direction
            differential_equations(&pos_x, &v_x, force_x, external_force_x, &max_x); // Calculate the drone's position and velocity in the x direction
            differential_equations(&pos_y, &v_y, force_y, external_force_y, &max_y); // Calculate the drone's position and velocity in the y direction

            printf("Drone Force (X,Y): %.2f,%.2f\t|\t",force_x,force_y); // Print the drone's force in the x and y direction
            printf("External Force (X,Y): %.2f,%.2f\n",external_force_x,external_force_y); // Print the external force in the x and y direction
            printf("X - Position: %.2f / Y - Position: %.2f\n", pos_x, pos_y); // Print the drone's position in the x and y direction
            printf("Velocity: %.2f / Velocity: %.2f\n", v_x, v_y); // Print the drone's velocity in the x and y direction
            printf("--------------------------------------------------------\n");
            fflush(stdout); // Flush the stdout buffer
            int x_f = (int)round(pos_x); // Round the position of the drone in the x direction
            int y_f = (int)round(pos_y); // Round the position of the drone in the y direction

            // -------------------------------------------------------------------- //
            /* Step 5: SEND THE DRONE POSITION TO SERVER */
            char position_msg[MSG_LEN]; // Message to be sent to the server
            sprintf(position_msg, "D:%d,%d", x_f, y_f); // Send the drone's position to the server
            write_to_pipe(drone_server[1], position_msg); // Send the drone's position to the server
        }

        // Introduce a delay on the loop to simulate real-time intervals.
        usleep(TIME_INTERVAL * 1e6); 
    }

    return 0; 
}


// Implementation of the Differential Equations function
void differential_equations(double *position, double *velocity, double force, double external_force, double *max_pos)
{
    double total_force = force + external_force; // The total force on the drone
    double acceleration_x = (total_force - DAMPING * (*velocity)) / MASS; // acceleration of the drone
    *velocity = *velocity + acceleration_x * TIME_INTERVAL; // velocity of the drone
    *position = *position + (*velocity) * TIME_INTERVAL; // position of the drone

    // Walls are the maximum position the drone can reach
    if (*position < 0) 
    { 
        *position = 0; // If the drone reaches a wall, set the position to 0
    } 
    if (*position > *max_pos) 
    { 
        *position = *max_pos - 1; // If the drone reaches a wall, set the position to the maximum position - 1
    }

}

// Moving the drone step by step as initial development
void step_method(int *x, int *y, int action_x, int action_y)
{
    (*x) = (*x) + action_x;
    (*y) = (*y) + action_y;
}


void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y)
{
    // Calculate the distance between the current position drone and the target
    double distance_to_target = sqrt(pow(target_x - drone_x, 2) + pow(target_y - drone_y, 2));
    double angle_to_target = atan2(target_y - drone_y, target_x - drone_x);

    // In order to prevent division by zero or exceedingly high forces
    if (distance_to_target < min_distance)
    {
        // Maintain the force value for the minimum distance.
        distance_to_target = min_distance;
    }
    else if (distance_to_target < start_distance)
    {
        // Maintain the force value for the start distance and increment the distance counter
        *external_force_x += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * cos(angle_to_target);
        *external_force_y += Coefficient * (1.0 / distance_to_target - 1.0 / 5.0) * (1.0 / pow(distance_to_target,2)) * sin(angle_to_target);
    }
    else
    {
        // if the distance is something else, set the force value to zero
        *external_force_x += 0;
        *external_force_y += 0;
    }
    
    // Determine the REPULSION force exerted by the obstacle
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
    int total_obstacles; // The total number of obstacles
    sscanf(obstacles_msg, "O[%d]", &total_obstacles); // Get the total number of obstacles

    char *token = strtok(obstacles_msg + 4, "|"); // Get the first token
    *number_obstacles = 0; // Initialize the number of obstacles to 0

    while (token != NULL && *number_obstacles < total_obstacles) // While the token is not NULL and the number of obstacles is less than the total number of obstacles
    {
        sscanf(token, "%d,%d", &obstacles[*number_obstacles].x, &obstacles[*number_obstacles].y);
        obstacles[*number_obstacles].total = *number_obstacles + 1;

        token = strtok(NULL, "|");
        (*number_obstacles)++;
    }
}

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
    }
}

void get_args(int argc, char *argv[])
{
    
    sscanf(argv[1], "%d %d %d", &server_drone[0], &drone_server[1], &lowest_target_fd[0]); // Get the drone server's pipe file descriptors
}

void force_check_handler(double *item1, double *item2, double condition, double replacement)
{
    if (*item1 > condition)
    {
        *item1 = replacement;
    }
    if (*item2 > condition)
    {
        *item2 = replacement;
    }
    if (*item1 < -condition)
    {
        *item1 = replacement;
    }
    if (*item2 < -condition)
    {
        *item2 = replacement;
    }
}