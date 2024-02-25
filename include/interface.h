#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>
#include "constants.h"

int find_lowest_ID(Targets *targets, int number_targets);

void remove_target(Targets *targets, int *number_targets, int index_to_remove);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void get_args(int argc, char *argv[]);

void parse_target_message(char *targets_msg, Targets *targets, int *number_targets, Targets *original_targets);

int check_collision_drone_obstacle (Obstacles obstacles[], int number_obstacles, int drone_x, int drone_y);

void parse_obstacles_message(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);

void draw_window(int drone_x, int drone_y, Targets *targets, int number_targets, Obstacles *obstacles, int number_obstacles, const char *score_msg);

void draw_final_window(int score);

char* determine_action(int pressed_key, char *shared_action);

void calculate_score(int *counter, int *score, int operator);


#endif //INTERFACE_H