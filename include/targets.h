#ifndef TARGET_H
#define TARGET_H

// User-defined parameters
#define SCREEN_WIDTH 168
#define SCREEN_HEIGHT 44
#define MAX_TARGETS 8

// Structure to represent a target
typedef struct {
    int x;
    int y;
} Target;

/**
 * get_args function takes in the number of arguments and the arguments themselves as strings
 * and sets up the global variables with the values passed in as arguments
 * 
 * @param argc number of arguments passed in
 * @param argv array of strings containing the arguments
 */
void get_args(int argc, char *argv[]) ;
// Function to generate random coordinates within a given sector
/**
 * generate_random_cordinates function generates random coordinates within a given sector
 * 
 * @param sector_width width of the sector
 * @param sector_height height of the sector
 * @param x pointer to an integer to store the x coordinate
 * @param y pointer to an integer to store the y coordinate
 */
void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y);
/**
 * generate_random_order function generates a random order of integers from 0 to max_targets - 1
 * 
 * @param order pointer to an integer array to store the order
 * @param max_targets the maximum number of targets
 */

void generate_random_order(int *order, int max_targets);
/**
 * generate_target function generates a random target within the screen boundaries
 * 
 * @param order the index of the target in the target order
 * @param screen_size_x the width of the screen
 * @param screen_size_y the height of the screen
 * @return the generated target
 */
Target generate_target(int order, int screen_size_x, int screen_size_y);
/**
 * make_target_msg function takes in a pointer to an array of targets and a character array as input, 
 * and outputs a character array containing the coordinates of each target in the form of "x,y" separated by a space.
 * 
 * @param targets pointer to an array of targets
 * @param message pointer to a character array to store the output message
 */
void make_target_msg(Target *targets, char *message);


#endif // TARGET_H