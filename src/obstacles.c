#include "obstacles.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>


// User-defined parameters
#define MAX_OBSTACLES 7
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100
#define MIN_SPAWN_TIME 2
#define MAX_SPAWN_TIME 7
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)


int main()
{
    srand(time(NULL));
    Obstacle obstacles[MAX_OBSTACLES];
    int number_obstacles = 0;
    char obstacles_msg[1000]; // Adjust the size based on your requirements

    while (1)
    {
        // Check if it's time to spawn a new obstacle
        if (number_obstacles < MAX_OBSTACLES)
        {
            Obstacle new_obstacle;
            new_obstacle.x = rand() % SCREEN_WIDTH;
            new_obstacle.y = rand() % SCREEN_HEIGHT;
            new_obstacle.spawn_time = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);

            obstacles[number_obstacles] = new_obstacle;
            number_obstacles++;
        }

        // Print obstacles every second
        obstacles_msg[0] = '\0'; // Clear the string
        print_obstacles (obstacles, number_obstacles, obstacles_msg);
        sleep(WAIT_TIME);

        // Check if any obstacle has exceeded its spawn time
        time_t current_time = time(NULL);
        for (int i = 0; i < number_obstacles; i++)
        {
            if (obstacles[i].spawn_time <= current_time)
            {
                // Replace the obstacle with a new one
                obstacle[i].x = rand() % SCREEN_WIDTH;
                obstacle[i].y = rand() % SCREEN_HEIGHT;
                obstacle[i].spawn_time = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
            }
        }

    }

    return 0;
    
}

void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[])
{
    // Append the letter O and the total number of obstacles to obstacles_msg
    sprintf(obstacles_msg, "0[%d]", number_obstacles);

    for (int i = 0; i < number_obstacles, i++)
    {
        // Append obstacle information to obstacles_msg
        sprintf(obstacles_msg + strlen(obstacles_msg), "%d, %d", obstacles[i].x, obstacles[i].y);

        // Add a separator if there are more obstacles
        if (i < number_obstacles - 1)
        {
            sprintf(obstacles_msg + strlen(obstacles_msg), "|");
        }
    }
    printf("%s\n", obstacles_msg);
}


