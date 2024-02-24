#ifndef OBSTACLES_H
#define OBSTACLES_H

// User-defined parameters
#define MAX_OBSTACLES 7
#define MIN_SPAWN_TIME 8
#define MAX_SPAWN_TIME 15
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)


#include "constants.h"
#include <time.h>

typedef struct {
    int x;
    int y;
    time_t spawn_time;
} Obstacle;

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

void read_and_echo(int socket, char socket_msg[]);
int read_and_echo_non_blocking(int socket, char socket_msg[]);
void write_message_and_wait_for_echo(int socket, char socket_msg[], size_t msg_size);


#endif // OBSTACLES_H