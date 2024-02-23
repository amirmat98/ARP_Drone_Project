#ifndef DRONE_H
#define DRONE_H

#include "constants.h"
#include "signal.h"
#include "../include/targets.h"

#define SLEEP_DRONE 100000  // To let the interface.c process execute first write the initial positions.

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define D_T 0.1 // Time interval in seconds
#define F_MAX 30.0 // Maximal force that can be acted on the drone's motors
#define EXT_FORCE_MAX 40.0 // Maximum external force on any axis.

// New Constants 
#define Coefficient 400.0 // This was obtained by trial-error
#define min_distance 2.0
#define start_distance 5.0

typedef struct{
    double position_x;
    double position_y;
    double velocity_x;
    double velocity_y;
    double force_x;
    double force_y;
    double external_force_x;
    double external_force_y;
} Drone;

/**
 * get_args function
 * 
 * This function is used to get the arguments passed to the program from the command line.
 * 
 * @param argc argument count
 * @param argv argument vector
 */
void get_args(int argc, char *argv[]);

/**
 * signal_handler function
 * 
 * This function is used to handle any signals sent to the program.
 * 
 * @param signo signal number
 * @param siginfo signal information
 * @param context signal context
 */
void signal_handler(int signo, siginfo_t *siginfo, void *context);
/**
 * calculate_extenal_force function
 * 
 * This function calculates the external force acting on the drone.
 * 
 * @param drone_x x-coordinate of the drone
 * @param drone_y y-coordinate of the drone
 * @param target_x x-coordinate of the target
 * @param target_y y-coordinate of the target
 * @param obstacle_x x-coordinate of the obstacle
 * @param obstacle_y y-coordinate of the obstacle
 * @param external_force_x pointer to the x-coordinate of the external force
 * @param external_force_y pointer to the y-coordinate of the external force
 */
void calculate_extenal_force(Drone *drone, Target *target, Obstacles *obstacle);

void parse_obstacles_msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);
/**
 * differential_equations function
 * 
 * This function implements the differential equations of motion for the quadcopter.
 * 
 * @param position pointer to the x-coordinate of the drone
 * @param velocity pointer to the y-coordinate of the drone
 * @param force the force acting on the drone
 * @param external_force the external force acting on the drone
 * @param max_pos pointer to the maximum position of the drone
 */
void differential_equations(Drone *drone, double screen_x, double screen_y);



#endif //DRONE_H