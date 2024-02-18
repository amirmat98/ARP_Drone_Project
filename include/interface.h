#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>
#include "constants.h"


void signal_handler(int signo, siginfo_t *siginfo, void *context);
void get_args(int argc, char *argv[]);



void draw_window(int drone_x, int drone_y, Targets *targets, int number_targets, Obstacles *obstacles, int number_obstacles, const char *score_msg);
// void handle_input(int *shared_key, sem_t *semaphore);


int find_lowest_ID(Targets *targets, int number_targets);
void remove_target(Targets *targets, int *number_targets, int index_to_remove);
void parse_obstacles_message(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles);
void parse_target_message(char *targets_msg, Targets *targets, int *number_targets, Targets *original_targets);

void draw_final_window(int score);

int check_collision_drone_obstacle (Obstacles obstacles[], int number_obstacles, int drone_x, int drone_y);
void update_score(int *score, int *counter);

char* determine_action(int pressed_key, char *shared_action);

int decypher_message(char server_msg[]);

#endif //INTERFACE_H