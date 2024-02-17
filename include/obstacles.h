#ifndef OBSTACLES_H
#define OBSTACLES_H

// User-defined parameters
#define MAX_OBSTACLES 7
// #define SCREEN_WIDTH 100
// #define SCREEN_HEIGHT 100
#define MIN_SPAWN_TIME 4
#define MAX_SPAWN_TIME 10
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)


struct Obstacle
{
    int x;
    int y;
    time_t spawn_time;
};

void get_args(int argc, char *argv[]);
void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[]);

#endif // OBSTACLES_H