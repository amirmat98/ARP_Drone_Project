#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>


void signal_handler(int signo, siginfo_t *siginfo, void *context);
void get_args(int argc, char *argv[]);


struct Obstacles
{
    int total;
    int x;
    int y;
};

struct Targets
{
    int ID;
    int x;
    int y;
};



void draw_window(int max_x, int max_y, int drone_x, int drone_y, Targets *targets, int number_targets, Obstacles *obstacles, int number_obstacles, const char *score_msg);
void handle_input(int *shared_key, sem_t *semaphore);


int find_lowest_ID(Targets *targets, int number_targets);
void remove_target(Targets *targets, int *number_targets, int index_to_remove);
void parse_obstacles_message(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);
void parse_target_message(char *targets_msg, Targets *targets, int *number_targets);

int check_collision_drone_obstacle (Obstacles obstacles[], int number_obstacles, int drone_x, int drone_y);
void update_score(int *score, int *counter);

char* determine_action(int pressed_key, char *shared_action);

#endif //INTERFACE_H