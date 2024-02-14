#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>


void create_window(int max_x, int max_y);
void draw_drone(int drone_x, int drone_y);
void handle_input(int *shared_key, sem_t *semaphore);


#endif //INTERFACE_H