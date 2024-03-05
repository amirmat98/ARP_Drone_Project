#ifndef DRONE_H
#define DRONE_H

#include "constants.h"
#include "signal.h"

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define D_T 0.1 // Time interval in seconds
#define F_MAX 30.0 // Maximum force that can be acted on the drone's motors
#define EXT_FORCE_MAX 40.0 // Maximum external force on any axis.

// New Constants 
#define COEFFICIENT 400.0 // Obtained by testing and trial-error
#define MIN_DISTANCE 5.0  // Distance where the drone begins feeling force of an object
#define MAX_DISTANCE 2.0  // Distance where the drone caps out the force of an object
#define FLOAT_TOLERANCE 0.0044  // Below this value the process drone.c will not print the velocity anymore


void get_args(int argc, char *argv[]);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y);

void parse_obstacles_msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);

void differential_equations(double *position, double *velocity, double force, double external_force, double *max_pos);

void boundrey_check_handler(double *x, double *y, double condition);


#endif //DRONE_H