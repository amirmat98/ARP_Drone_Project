#ifndef DRONE_H
#define DRONE_H

#include "constants.h"
#include "signal.h"


void differential_equations(double *position, double *velocity, double force, double external_force, double *max_pos);
void step_method(int *x, int *y, int action_x, int action_y);

void calculate_extenal_force(double drone_x, double drone_y, double target_x, double target_y, double obstacle_x, double obstacle_y, double *external_force_x, double *external_force_y);
void parse_obstacles_Msg(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);

#endif //DRONE_H