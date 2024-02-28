#include "obstacles.h"
#include "import.h"


// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];

int main(int argc, char *argv[])
{
    get_args(argc, argv);

    // Seed the random number generator
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];

    int number_obstacles = 0;
    char obstacles_msg[MSG_LEN]; // Adjust the size based on your requirements

    // To compare previous values
    bool obtained_dimensions = false;


    // Variables
    int screen_size_x; 
    int screen_size_y;

    while (1)
    {
        
        /* Step 1: READ THE DATA FROM SERVER*/
        
        // Read data from the server
        char server_msg[MSG_LEN] = {0};

        if (read_from_pipe(server_obstacles[0], server_msg))
        {
            // Process the message received from the server.
            receive_message_from_server(server_msg, &screen_size_x, &screen_size_y);
            // Indicate that the screen dimensions have been received.
            obtained_dimensions = true;
        }

        // Continue the loop if the screen dimensions have not been obtained yet.
        if(!obtained_dimensions)
        {
            continue;
        }

        /* Step 2: OBSTACLES GENERATION & SEND DATA */

        if (number_obstacles < MAX_OBSTACLES) 
        {
            obstacles[number_obstacles] = generate_obstacle(screen_size_x, screen_size_y);
            number_obstacles++;
        }

        obstacles_msg[0] = '\0'; // Clear the string

        print_obstacles(obstacles, number_obstacles, obstacles_msg);

        sleep(WAIT_TIME);

        // Check if any obstacle has exceeded its spawn time
        check_obstacles_spawn_time(obstacles, number_obstacles, screen_size_x, screen_size_y);

        // SEND DATA TO SERVER
        write_to_pipe(obstacles_server[1],obstacles_msg);

    }

    return 0;
    
}

void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[])
{
    // Append the letter O and the total number of obstacles to obstacles_msg
    sprintf(obstacles_msg, "O[%d]", number_obstacles);

    for (int i = 0; i < number_obstacles; i++)
    {
        // Append obstacle information to obstacles_msg
        sprintf(obstacles_msg + strlen(obstacles_msg), "%.3f,%.3f", (float)obstacles[i].x, (float)obstacles[i].y);
        // Add a separator if there are more obstacles
        if (i < number_obstacles - 1)
        {
            sprintf(obstacles_msg + strlen(obstacles_msg), "|");
        }
    }
    printf("%s\n", obstacles_msg);
    fflush(stdout);
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_obstacles[0], &obstacles_server[1]);
}

void receive_message_from_server(char *message, int *x, int *y)
{
    printf("Obtained from server: %s\n", message);
    float temp_scx, temp_scy;
    sscanf(message, "I2:%f,%f", &temp_scx, &temp_scy);
    *x = (int)temp_scx;
    *y = (int)temp_scy;
    // printf("Screen size: %d x %d\n", *x, *y);
    fflush(stdout);
}

Obstacle generate_obstacle(int x, int y)
{
    Obstacle obstacle;
    obstacle.x = (int)(0.05 * x) + (rand() % (int)(0.95 * x));
    obstacle.y = (int)(0.05 * y) + (rand() % (int)(0.95 * y));
    obstacle.spawn_time = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
    return obstacle;
}

void check_obstacles_spawn_time(Obstacle obstacles[], int number_obstacles, int x, int y)
{
    time_t currentTime = time(NULL);
    for (int i = 0; i < number_obstacles; ++i)
    {
        if (obstacles[i].spawn_time <= currentTime)
        {
            // Replace the obstacle with a new one
            obstacles[i] = generate_obstacle(x, y);
        }
    }
}