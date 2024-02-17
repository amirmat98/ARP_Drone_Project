#ifndef TARGET_H
#define TARGET_H

// User-defined parameters
#define SCREEN_WIDTH 168
#define SCREEN_HEIGHT 44
#define MAX_TARGETS 8

// Structure to represent a target
struct Target
{
    int x;
    int y;
};

void get_args(int argc, char *argv[]);
// Function to generate random coordinates within a given sector
void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y);


#endif // TARGET_H