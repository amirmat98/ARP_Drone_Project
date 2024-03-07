#ifndef OBSTACLES_H
#define OBSTACLES_H

#include <time.h>
#include <signal.h>

// User-defined parameters
#define MAX_OBSTACLES 8
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


/**
 * print_obstacles - This function prints the obstacles
 * @obstacles: Array of obstacles
 * @number_obstacles: Number of obstacles
 * @obstacles_msg: Message to be printed before the obstacles
 *
 * This function prints the obstacles in the array, along with the message
 * specified by the @obstacles_msg parameter.
 */
void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[]);


/**
 * receive_message_from_server - This function receives a message from the server
 * @message: The message received from the server
 * @x: The x-coordinate of the ball
 * @y: The y-coordinate of the ball
 *
 * This function receives a message from the server and extracts the x- and y-coordinates of the ball.
 */
void receive_message_from_server(char *message, int *x, int *y);


/**
 * generate_obstacle - This function generates a new obstacle
 * @x: The x-coordinate of the obstacle
 * @y: The y-coordinate of the obstacle
 *
 * This function generates a new obstacle with the specified @x and @y coordinates.
 * The obstacle's spawn time is set to a random value between MIN_SPAWN_TIME and
 * MAX_SPAWN_TIME.
 *
 * Returns: The generated obstacle
 */
Obstacle generate_obstacle(int x, int y);


/**
 * check_obstacles_spawn_time - This function checks if the spawn time of an obstacle has been reached
 * @obstacles: Array of obstacles
 * @number_obstacles: Number of obstacles
 * @x: The x-coordinate of the ball
 * @y: The y-coordinate of the ball
 *
 * This function iterates through the array of obstacles and checks if the spawn time of each obstacle has been reached. If an obstacle's spawn time has been reached, a new obstacle is generated with a random x- and y-coordinate.
 */
void check_obstacles_spawn_time(Obstacle obstacles[], int number_obstacles, int x, int y);


/**
 * signal_handler - This function handles signals sent to the process
 * @signo: The signal number
 * @siginfo: Struct containing additional information about the signal
 * @context: Context information about the signal
 *
 * This function handles signals sent to the process, such as SIGINT (Ctrl+C) or SIGTERM.
 * It prints a message and exits the program.
 */
void signal_handler(int signo, siginfo_t *siginfo, void *context);


/**
 * stop_message_handler - This function handles the stop message from the server
 * @message: The message received from the server
 *
 * This function handles the stop message from the server by printing a message and exiting the program.
 */
void stop_message_handler(char *message);


#endif // OBSTACLES_H