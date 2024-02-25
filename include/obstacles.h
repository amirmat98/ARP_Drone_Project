#ifndef OBSTACLES_H
#define OBSTACLES_H

#include <time.h>
#include <signal.h>

// User-defined parameters
#define MAX_OBSTACLES 5
#define MIN_SPAWN_TIME 10
#define MAX_SPAWN_TIME 15
#define WAIT_TIME 1 // How often a new obstacle is created (if < MAX_OBSTACLES)

// Structure to represente a single obstacle
typedef struct {
    int x;
    int y;
    time_t spawn_time;
} Obstacle; // Diferent from "Obstacles" in constants.h

/**
 * get_args - This function gets the arguments from the command line
 * @argc: Number of arguments
 * @argv: Array of arguments
 *
 * This function gets the arguments from the command line and stores them in the
 * global variables.
 */
void get_args(int argc, char *argv[]);
void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[]);

void receive_message_from_server(char *message, int *x, int *y);

Obstacle generate_obstacle(int x, int y);

void check_obstacles_spawn_time(Obstacle obstacles[], int number_obstacles, int x, int y);

void signal_handler(int signo, siginfo_t *siginfo, void *context);


#endif // OBSTACLES_H