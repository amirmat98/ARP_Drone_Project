#ifndef DRONE_H
#define DRONE_H


void differential_equations(double *position, double *velocity, double force, double *max_pos)
void step_method(int *x, int *y, int action_x, int action_y);

#endif //DRONE_H