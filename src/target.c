#include "targets.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <signal.h>

void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y)
{
    *x = rand() % sector_width;
    *y = rand() % sector_height;
}

int main()
{
    int counter = 0;
    while(1)
    {
        // Set seed for random number generation
        srand(time(NULL));

        const int sector_width = SCREEN_WIDTH / 3;
        const int sector_height = SCREEN_HEIGHT / 2;

        // Array to store generated targets
        Target targets[MAX_TARGETS];

        // Generate random order for distributing targets across sectors
        int order[MAX_TARGETS];
        for (int i = 0; i < MAX_TARGETS; i++)
        {
            order[i] = i;
        }
        for (int i = MAX_TARGETS - 1; i > 0; i--)
        {
            int j = rand() % (i+1);
            int temp = order[i];
            order[i] = order[j];
            order[j] = temp;
        }

        // String variable to store the targets in the specified format
        char targets_msg[100];

        // Generate targets within each sector
        for (int i = 0; i < MAX_TARGETS; i++)
        {
            Target target;

            // Determine sector based on the random order
            int sector_x = (order[i] % 3) * sector_width;
            int sector_y = (oreder[i] / 3) * sector_height;

            // Generate random coordinates within the sector
            generate_random_cordinates(sector_width, sector_height, &target.x, &target.y);

            // Adjust coordinates based on the sector position
            target.x += sector_x;
            target.y += sector_y;

            // Ensure coordinates do not exceed the screen size
            target.x = target.x % SCREEN_WIDTH;
            target.y = target.y % SCREEN_HEIGHT;

            // Store the target
            targets[i] = target;
        }

        // Construct the targets_msg string
        int offset = sprintf(targets_msg, "T[%d]", MAX_TARGETS);
        for (int i = 0; i < MAX_TARGETS; i++)
        {
            offset += sprintf(target_msg + offset, "%d , %d", target[i].x, target[i].y);

            // Add a separator unless it's the last element
            if (i < MAX_TARGETS - 1)
            {
                offset += sprintf (target_msg + offset, "|");
            }
        }
        target_msg[offset] = '\0'; // Null-terminate the string

        // Print the generated targets in the specified format
        printf ("%s\n", target_msg);

        sleep(1);
    }
    return 0;



}