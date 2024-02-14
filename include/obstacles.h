#ifndef OBSTACLES_H
#define OBSTACLES_H

struct Obstacle
{
    int x;
    int y;
    time_t spawn_time;
};

void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[]);

#endif // OBSTACLES_H