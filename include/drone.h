#ifndef DRONE_H
#define DRONE_H

struct Obstacles
{
    int total;
    int x;
    int y;
};


void differential_equations(double *position, double *velocity, double force, double *max_pos);
void step_method(int *x, int *y, int action_x, int action_y);

#endif //DRONE_H