#include "targets.h"
#include "import.h"

// Pipe file descriptors for reading from the server and writing to the server
int server_targets[0];
int targets_server[1];

int main(int argc, char *argv[])
{
    get_args(argc, argv);

    // Flags to track whether dimensions and targets have been obtained
    bool obtained_dimensions = false;
    bool targets_created = false;;

    // Variables
    int screen_size_x; int screen_size_y;
    float scale_x;
    float scale_y;
    int counter = 0;


    while(1)
    {
        /* STEP 1: READ THE DATA FROM SERVER*/

        // Message buffer for communication with the server
        char server_msg[MSG_LEN] = {0};
        // Check if there is data to read from the server and handle it
        if (read_from_pipe(server_targets[0], server_msg))
        {
            data_from_server_handler(server_msg, &screen_size_x, &screen_size_y);
            obtained_dimensions = true;
        }
        // Skip the remaining steps if dimensions are not obtained
        if(obtained_dimensions == false)
        {
            continue;
        }

        /* Step 2: Generate targets and send data*/
            
        // Array to store generated targets
        Target targets[MAX_TARGETS];
        // Set seed for random number generation
        srand(time(NULL));

        // Check if targets have been created
        if(targets_created == false)
        {
            // Generate random order for distributing targets across sectors
            int order[MAX_TARGETS];
            generate_random_order(order, MAX_TARGETS);

            // Generate targets within each sector
            for (int i = 0; i < MAX_TARGETS; ++i) 
            {
                // Generate the target
                Target temp_target = generate_target(order[i], screen_size_x, screen_size_y);
                // Store the target
                targets[i] = temp_target;
            }

            // Create target message
            char targets_msg[MSG_LEN];
            make_target_msg(targets, targets_msg);

            // Send the data to the server
            write_to_pipe(targets_server[1], targets_msg);
            targets_created = true;
        }

        // Delay
        usleep(5 * SLEEP_DELAY);  

    }


    return 0;

}

void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y)
{
    // Generate random coordinates within the sector boundaries
    *x = (int)(0.05 * sector_width) + (rand() % (int)(0.95 * sector_width));
    *y = (int)(0.05 * sector_height) + (rand() % (int)(0.95 * sector_height));
}

void get_args(int argc, char *argv[])
{
    // Parse command line arguments to obtain pipe file descriptors
    sscanf(argv[1], "%d %d", &server_targets[0], &targets_server[1]);
}

void generate_random_order(int *order, int max_targets)
{
    // Generate a random order of target indices
    for (int i = 0; i < max_targets; ++i) 
    {
        order[i] = i;
    }
    for (int i = max_targets - 1; i > 0; --i) 
    {
        int j = rand() % (i + 1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }
}

Target generate_target(int order, int screen_size_x, int screen_size_y)
{
    // Generate a target based on the order and screen dimensions
    Target target;

    // Sector dimensions
    const int sector_width = screen_size_x / 3;
    const int sector_height = screen_size_y / 2;

    // Determine sector based on the random order
    int sector_x = (order % 3) * sector_width;
    int sector_y = (order / 3) * sector_height;

    // Generate random coordinates within the sector
    generate_random_cordinates(sector_width, sector_height , &target.x, &target.y);

    // Adjust coordinates based on the sector position
    target.x += sector_x;
    target.y += sector_y;

    // Ensure coordinates do not exceed the screen size
    target.x = target.x % screen_size_x;
    target.y = target.y % screen_size_y;

    return target;

}

void make_target_msg(Target *targets, char *message)
{
    // Construct the targets_msg string
    int offset = sprintf(message, "T[%d]", MAX_TARGETS);
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        offset += sprintf(message + offset, "%.3f,%.3f", (float)targets[i].x, (float)targets[i].y);

        // Add a separator unless it's the last element
        if (i < MAX_TARGETS - 1)
        {
            offset += sprintf(message + offset, "|");
        }
    }
    message[offset] = '\0'; // Null-terminate the string
    //printf("%s\n", message);
}

void data_from_server_handler(char *message, int *screen_x, int *screen_y)
{
    float temp_scx, temp_scy;
    sscanf(message, "I2:%f,%f", &temp_scx, &temp_scy);
    *screen_x = (int)temp_scx;
    *screen_y = (int)temp_scy;
    //printf("Obtained from server: %s\n", message);
    fflush(stdout);
} 