#ifndef DRONE_H
#define DRONE_H

#include "constants.h"
#include "signal.h"

#define SLEEP_DRONE 100000  // To let the interface.c process execute first write the initial positions.

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define D_T 0.1 // Time interval in seconds
#define F_MAX 30.0 // Maximal force that can be acted on the drone's motors
#define EXT_FORCE_MAX 40.0 // Maximum external force on any axis.

// New Constants 
#define Coefficient 400.0 // This was obtained by trial-error
#define min_distance 1.0
#define start_distance 5.0


void get_args(int argc, char *argv[]);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y);

void parse_obstacles_msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);

void differential_equations(double *position, double *velocity, double force, double external_force, double *max_pos);


#endif //DRONE_H