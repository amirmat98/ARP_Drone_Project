#include "constants.h"
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

// Pipes working with the server
int server_targets[0];
int targets_server[1];

int main(int argc, char *argv[])
{
    get_args(argc, argv);

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // To compare previous values
    int obtained_dimensions = 0;
    int targets_created = 0;

    // Variables
    int screen_size_x; int screen_size_y;
    float scale_x;
    float scale_y;

    int counter = 0;

    while(1)
    {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_targets[0], &read_fds);
        
        char server_msg[MSG_LEN];

        int ready = select(server_targets[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) 
        {
            perror("Error in select");
        }

        if (ready > 0 && FD_ISSET(server_targets[0], &read_fds)) 
        {
            ssize_t bytes_read = read(server_targets[0], server_msg, MSG_LEN);
            if (bytes_read > 0) 
            {
                sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                printf("Obtained from server: %s\n", server_msg);
                fflush(stdout);
                obtained_dimensions = 1;
            }
        }
        if(obtained_dimensions == 0)
        {
            continue;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: TARGET GENERATION & SEND DATA*/
        /////////////////////////////////////////////////////
        
        // Array to store generated targets
        Target targets[MAX_TARGETS];
        // Set seed for random number generation
        srand(time(NULL));

        if(targets_created == 0)
        {
            // Sector dimensions
            const int sector_width = screen_size_x / 3;
            const int sector_height = screen_size_y / 2;

            // Generate random order for distributing targets across sectors
            int order[MAX_TARGETS];
            for (int i = 0; i < MAX_TARGETS; ++i) 
            {
                order[i] = i;
            }
            for (int i = MAX_TARGETS - 1; i > 0; --i) 
            {
                int j = rand() % (i + 1);
                int temp = order[i];
                order[i] = order[j];
                order[j] = temp;
            }

            // String variable to store the targets in the specified format
            char targets_msg[MSG_LEN]; // Adjust the size accordingly

            // Generate targets within each sector
            for (int i = 0; i < MAX_TARGETS; ++i) 
            {
                Target target;

                // Determine sector based on the random order
                int sector_x = (order[i] % 3) * sector_width;
                int sector_y = (order[i] / 3) * sector_height;

                // Generate random coordinates within the sector
                generate_random_cordinates(sector_width, sector_height, &target.x, &target.y);

                // Adjust coordinates based on the sector position
                target.x += sector_x;
                target.y += sector_y;

                // Ensure coordinates do not exceed the screen size
                target.x = target.x % screen_size_x;
                target.y = target.y % screen_size_y;

                // Store the target
                targets[i] = target;
            }

            // Construct the targets_msg string
            int offset = sprintf(targets_msg, "T[%d]", MAX_TARGETS);
            for (int i = 0; i < MAX_TARGETS; ++i) 
            {
                offset += sprintf(targets_msg + offset, "%d,%d", targets[i].x, targets[i].y);

                // Add a separator unless it's the last element
                if (i < MAX_TARGETS - 1) 
                {
                    offset += sprintf(targets_msg + offset, "|");
                }
            }
            targets_msg[offset] = '\0'; // Null-terminate the string

            // Send the data to the server
            printf("%s\n", targets_msg);
            write_to_pipe(targets_server[1],targets_msg);
            targets_created = 1;
        }

        // Delay
        usleep(500000);  

    }


    return 0;

}

void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y)
{
    *x = rand() % sector_width;
    *y = rand() % sector_height;
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_targets[0], &targets_server[1]);
}