#include "map.h"
#include "import.h"
#include <ncurses.h>


// Serverless pipe
int interface_km[2];
int interface_drone[2];

// Array of file descriptors for a pipe that facilitates communication with the server.
// server_interface[0] is for reading from the pipe, and server_interface[1] is for writing to it.
int server_interface[2];
int interface_server[2];

char log_file[80];
char msg[1024];

int main(int argc, char *argv[])
{
    // Retrieve the file descriptors for the pipes by parsing the command-line arguments
    get_args(argc, argv);

    // Signals
    // Setup the signal handling structure for handling specific signals
    struct sigaction sa;
    // Assign the signal_handler function to execute when a signal is received
    sa.sa_sigaction = signal_handler;
    // Set flags to modify behavior of the signal - SA_SIGINFO allows signal_handler
    // to receive additional information about the signal.
    sa.sa_flags = SA_SIGINFO;
    // Register signal_handler as the handler function for SIGINT (Ctrl+C)
    sigaction (SIGINT, &sa, NULL); 
    // Register signal_handler as the handler function for SIGUSR1,
    // a user-defined signal typically used for application-specific purposes. 
    sigaction (SIGUSR1, &sa, NULL);
    // Publish the current process ID (PID) to the watchdog using the WINDOW_SYM symbol.
    // This could be for registration or monitoring purposes, allowing the watchdog to
    // identify or keep track of this process.
    publish_pid_to_wd(WINDOW_SYM, getpid());

    //----------------------------------------------------------------------------------------//

    // INITIALIZATION AND EXECUTION OF NCURSES FUNCTIONS
    initscr(); // Initialize
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    noecho(); // Disable echoing: no key character will be shown when pressed.

    // Enable color
    start_color(); // Initialize color for drawing drone
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone will be of color blue
    init_pair(2, COLOR_RED, COLOR_BLACK);  // Obstacles are red
    init_pair(3, COLOR_GREEN, COLOR_BLACK); // Targets are green
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Score Text will be of color yellow
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

    //----------------------------------------------------------------------------------------//

    /* SET INITIAL DRONE POSITION */
    // Obtain the screen dimensions
    int screen_size_x;
    // int screen_size_x = 500;
    int screen_size_y;
    // int screen_size_y = 444;
    getmaxyx(stdscr, screen_size_y, screen_size_x);
     // Initial drone position will be the middle of the screen.
    int drone_x = screen_size_x / 2;
    int drone_y = screen_size_y / 2;
    float velocity_x = 0;
    float velocity_y = 0;

    // Write the initial position and screen size data into the server
    char initial_msg[MSG_LEN];
    sprintf(initial_msg, "I1:%d,%d,%d,%d", drone_x, drone_y, screen_size_x, screen_size_y);
    write_to_pipe(interface_server[1], initial_msg);

    //----------------------------------------------------------------------------------------//

    /* Useful variables creation*/

    // To compare current and previous data
    int obtained_targets = 0;
    int obtained_obstacles = 0;
    int prev_screen_size_y = 0;
    int prev_screen_size_x = 0;
    int original_screen_size_x;
    int original_screen_size_y;

    // Game logic
    char score_msg[MSG_LEN];
    int score = 0;
    int counter = 0;
    int timer_color = 0;

    // Targets and obstacles
    Targets targets[80];
    Targets original_targets[80];
    int number_targets;

    Obstacles obstacles[80];
    int number_obstacles;

    int iteration = 0;

    sprintf(msg,"Interface initialized");
    log_msg(log_file, INTERFACE, msg);

    //----------------------------------------------------------------------------------------//

    /* Intro message */
    show_intro_message(screen_size_x, screen_size_y);

    //----------------------------------------------------------------------------------------//

    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: SCREEN DIMENSIONS: SEND & RE-SCALE */
        /////////////////////////////////////////////////////

        getmaxyx(stdscr, screen_size_y, screen_size_x);
        /*SEND x,y  to server ONLY when screen size changes*/
        if (screen_size_x != prev_screen_size_x || screen_size_y != prev_screen_size_y) 
        {
            // To avoid division by 0 on first iteration
            if (iteration == 0)
            {
                prev_screen_size_x = screen_size_x;
                prev_screen_size_y = screen_size_y;
                original_screen_size_x = screen_size_x;
                original_screen_size_y = screen_size_y;
                iteration++;
            }
            // Send data
            char screen_msg[MSG_LEN];
            sprintf(screen_msg, "I2:%.3f,%.3f", (float)screen_size_x, (float)screen_size_y);
            write_to_pipe(interface_server[1], screen_msg);
            // Re-scale the targets on the screen
            if (iteration > 0 && obtained_targets == 1)
            {
                float scale_x = (float)screen_size_x / (float) original_screen_size_x;
                float scale_y = (float)screen_size_y / (float) original_screen_size_y;
                for (int i = 1; i < number_targets; i++) 
                {
                    targets[i].x = (int)((float)original_targets[i].x * scale_x);
                    targets[i].y = (int)((float)original_targets[i].y * scale_y);
                }
            }
            // Update previous values
            prev_screen_size_x = screen_size_x;
            prev_screen_size_y = screen_size_y;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////
        
        char server_msg[MSG_LEN];

        ssize_t bytes_read_drone = read(server_interface[0], server_msg, MSG_LEN);

        if (bytes_read_drone > 0) 
        {
            if (server_msg[0] == 'D')
            {
                sscanf(server_msg, "D:%d,%d", &drone_x, &drone_y);
            }
            else if (server_msg[0] == 'O')
            {
                parse_obstacles_message(server_msg, obstacles, &number_obstacles);
                obtained_obstacles = 1;
            }
            else if (server_msg[0] == 'T')
            {
                parse_target_message(server_msg, targets, &number_targets, original_targets);
                obtained_targets = 1;
            }
            
        }

        // Re-read the pipe if neither targets nor obstacles have been obtained
        if (obtained_targets == 0 || obtained_obstacles == 0)
        {
            continue;
        }

        // printf("TARGETS OBTAINED.\n");

        //////////////////////////////////////////////////////
        /* SECTION 3: DATA ANALYSIS & GAME SCORE CALCULATION*/
        /////////////////////////////////////////////////////


        // When there are no more targets, the score resets.
        if (number_targets == 0)
        {
            score = 0;
            obtained_targets = 0;
            write_to_pipe(interface_server[1], "GE");
            continue;
        }

        // Find the index of the target with the lowest ID
        int lowest_index = find_lowest_ID(targets, number_targets);

        // Obtain the coordinates of that target
        char drone_msg[MSG_LEN];
        sprintf(drone_msg, "%d,%d", targets[lowest_index].x, targets[lowest_index].y);
        
        // Send to drone w/ serverless pipe lowest_target
        write_to_pipe(interface_drone[1], drone_msg);

        if (targets[lowest_index].x == drone_x && targets[lowest_index].y == drone_y) 
        {
            
            calculate_score(counter, &score);

            counter = 0;
            // Remove the target with the lowest ID
            remove_target(targets, &number_targets, lowest_index);
        }

        // Check if the drone has crashed into an obstacle
        if (check_collision_drone_obstacle(obstacles, number_obstacles, drone_x, drone_y))
        {
            score -= 5;
            // calculate_score(counter, &score, 0);
        }

        // Counter/Timer linked to score calculations
        calculate_timer(counter, &timer_color);
        counter++;
        usleep(10000); // delay in seconds 

        //////////////////////////////////////////////////////
        /* SECTION 4: DRAW THE WINDOW & OBTAIN INPUT*/
        /////////////////////////////////////////////////////

        // Create a string for the player's current score
        sprintf(score_msg, "Score: %d", score);
        strcpy(msg, score_msg);
        log_msg(log_file, INTERFACE, msg);

        char timer_msg[MSG_LEN];
        sprintf(timer_msg, "%02d\"", (counter/10));
        // printf("%s\n", timer_msg);
        // Draws the window with the updated information of the terminal size, drone, targets, obstacles and score.
        draw_window(drone_x, drone_y, targets, number_targets, obstacles, number_obstacles, score_msg, timer_msg, timer_color);

        //////////////////////////////////////////////////////
        /* SECTION 5: HANDLE THE KEY PRESSED BY USER */
        /////////////////////////////////////////////////////

        int ch;
        if ((ch = getch()) != ERR)
        {
            char msg[MSG_LEN];
            sprintf(msg, "%c", ch);
            write_to_pipe(interface_km[1], msg);
        }
        flushinp(); // Clear the input buffer
    }

    // Clean up and finish up resources taken by ncurses
    endwin(); 

    return 0;
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d %s", &interface_km[1], &server_interface[0],
           &interface_server[1], &interface_drone[1], log_file);
}

// Function to handle signals within the process
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    if( signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT");
        log_msg(log_file, INTERFACE, msg);
        // Close file desciptors and exit
        close(interface_km[1]);
        close(interface_drone[1]);
        close(server_interface[0]);
        close(interface_server[1]);

        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}

// Function to draw the regular game window
void draw_window(int drone_x, int drone_y, Targets *targets, int number_targets, 
                    Obstacles *obstacles, int number_obstacles, const char *score_msg, const char *timer_msg, int timer_color)
{
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);

    // Draw border on the screen
    box(stdscr, 0, 0);

    // Print Timer
    if (timer_color == 3)
    {
        attron(COLOR_PAIR(3));
        mvprintw(0, (max_x/4) - 5, "%s", timer_msg);
        attroff(COLOR_PAIR(3));
    }
    else if (timer_color == 4)
    {
        attron(COLOR_PAIR(4));
        mvprintw(0, (max_x/4) - 5, "%s", timer_msg);
        attroff(COLOR_PAIR(4));
    }
    else if (timer_color == 5)
    {
        attron(COLOR_PAIR(5));
        mvprintw(0, (max_x/4) - 5, "%s", timer_msg);
        attroff(COLOR_PAIR(5));
    }
    else if (timer_color == 2)
    {
        attron(COLOR_PAIR(2));
        mvprintw(0, (max_x/4) - 5, "%s", timer_msg);
        attroff(COLOR_PAIR(2));
    }

    // Print a title in the top center part of the window
    mvprintw(0, (max_x - strlen(score_msg)) / 2, "%s", score_msg);

    // Print the cordination of Drone
    int new_x, new_y;
    convert_interface_cordination_to_standard(drone_x, drone_y, &new_x, &new_y);
    char temp_drone_msg[MSG_LEN];
    sprintf(temp_drone_msg, "Drone: X: %d, Y: %d", new_x, new_y);
    mvprintw(max_y - 1, (max_x - strlen(temp_drone_msg)) / 2, "%s", temp_drone_msg);

    // Draw a plus sign to represent the drone
    mvaddch(drone_y, drone_x, '+' | COLOR_PAIR(1));


    // Draw  targets and obstacles on the board
    for (int i = 0; i<number_obstacles; i++)
    {
        mvaddch(obstacles[i].y, obstacles[i].x, '*' | COLOR_PAIR(2));
    }

    for (int i = 0; i < number_targets; i++)
    {
        mvaddch(targets[i].y, targets[i].x, targets[i].ID + '0' | COLOR_PAIR(3));
    }

    refresh();
    
}


// Function to find the index of the target with the lowest ID
int find_lowest_ID(Targets *targets, int number_targets)
{
    int lowest_ID = targets[0].ID;
    int lowest_index = 0;
    for (int i = 1; i < number_targets; i++)
    {
        if (targets[i].ID < lowest_ID)
        {
            lowest_ID = targets[i].ID;
            lowest_index = i;
        }
    }
    return lowest_index;
}

// Function to remove a target at a given index from the array
void remove_target(Targets *targets, int *number_targets, int index_to_remove)
{
    // Shift elements to fill the gap
    for (int i = index_to_remove; i < (*number_targets - 1); i++)
    {
        targets[i] = targets[i+1];
    }

    // Decrement the number of targets
    (*number_targets)--;

}
// Function to extract the coordinates from the obstacles_msg string into the struct Obstacles.
void parse_obstacles_message(char *obstacles_msg, Obstacles *obstacles, int *number_obstacles)
{
    int total_obstacles;
    sscanf(obstacles_msg, "O[%d]", &total_obstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *number_obstacles = 0;

    while (token != NULL && *number_obstacles < total_obstacles)
    {
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);
         // Convert float to int (rounding is acceptable)
        obstacles[*number_obstacles].x = (int)(x_float + 0.5);
        obstacles[*number_obstacles].y = (int)(y_float + 0.5);

        obstacles[*number_obstacles].total = *number_obstacles + 1;

        token = strtok(NULL, "|");
        (*number_obstacles)++;
    }
}

// Function to extract the coordinates from the targets_msg string into the struct Targets.
void parse_target_message(char *targets_msg, Targets *targets, int *number_targets, Targets *original_targets)
{
    char *token = strtok(targets_msg + 4, "|");
    *number_targets = 0;

    while (token != NULL)
    {
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);

        // Convert float to int (rounding is acceptable)
        targets[*number_targets].x = (int)(x_float + 0.5);
        targets[*number_targets].y = (int)(y_float + 0.5);
        targets[*number_targets].ID = *number_targets + 1;

        // Save original values
        original_targets[*number_targets].x = targets[*number_targets].x;
        original_targets[*number_targets].y = targets[*number_targets].y;
        original_targets[*number_targets].ID = targets[*number_targets].ID;

        // Handle token
        token = strtok(NULL, "|");
        (*number_targets)++;
    }
}

// Function to check if drone is at the same coordinates as any obstacle.
int check_collision_drone_obstacle (Obstacles obstacles[], int number_obstacles, int drone_x, int drone_y)
{
    for (int i = 0; i < number_obstacles; i++)
    {
        if (obstacles[i].x == drone_x && obstacles[i].y == drone_y)
        {
            return 1; // Drone is at the same coordinates as an obstacle
        }
    }
    return 0; // Drone is not at the same coordinates as any obstacle
}


void calculate_score(int counter, int *score)
{
    // Update score and counter (reset timer)
    if (counter > 1500) 
    {  // 1500 * 10ms = 15 seconds 
        *score += 2;
    }
    else if (counter > 1000) 
    {   // 1000 * 10ms = 10 seconds
        *score += 4;
    }
    else if (counter > 500) 
    {   // 500 * 10ms = 5 seconds 
        *score += 6;
    }
    else 
    {  // Les than 5 seconds
        *score += 10;
    }
}

void calculate_timer(int counter, int *timer_color)
{
        // Update score and counter (reset timer)
    if (counter > 400) 
    {  // 1500 * 10ms = 15 seconds 
        *timer_color = 2;
    }
    else if (counter > 250) 
    {   // 1000 * 10ms = 10 seconds
        *timer_color = 5;
    }
    else if (counter > 150) 
    {   // 500 * 10ms = 5 seconds 
        *timer_color = 4;
    }
    else 
    {  // Les than 5 seconds
        *timer_color = 3;
    }
}

void convert_interface_cordination_to_standard(int x, int y, int *new_x, int *new_y)
{
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);
    *new_x = x - (max_x / 2);
    *new_y = (max_y / 2) - y;
}

void show_intro_message(int x, int y)
{
    clear();

    char intro_msg[MSG_LEN];

    sprintf(intro_msg, "Welcome to the Drone Game!");
    attron(COLOR_PAIR(1));
    mvprintw((10), (x - strlen(intro_msg)) / 2, "%s", intro_msg);
    attroff(COLOR_PAIR(1));

    sprintf(intro_msg, "This game is designed and implemented by AmirMahdi Matin - s5885715@studenti.unige.it");
    attron(COLOR_PAIR(3));
    mvprintw((y - 10), (x - strlen(intro_msg)) / 2, "%s", intro_msg);
    attroff(COLOR_PAIR(3));  


    sprintf(intro_msg, "Use 'W' and 'X' for Up and Down of the Drone");
    mvprintw((y/2 - 10), (x - strlen(intro_msg)) / 2, "%s", intro_msg);

    sprintf(intro_msg, "Use 'A' and 'D' for Left and Right of the Drone");
    mvprintw((y/2 - 9), (x - strlen(intro_msg)) / 2, "%s", intro_msg);

    sprintf(intro_msg, "Use 'S' to stop the Drone");
    mvprintw((y/2 - 8), (x - strlen(intro_msg)) / 2, "%s", intro_msg);

    sprintf(intro_msg, "Use 'H' for abort the movment of the Drone in emergency mode");
    mvprintw((y/2 - 7), (x - strlen(intro_msg)) / 2, "%s", intro_msg);
    
    refresh();

    for (int i = 0; i <= 10; i++)
    {
        sprintf(intro_msg, "The game will start in %d seconds", 10 - i);
        attron(COLOR_PAIR(2));
        mvprintw((y/2), (x - strlen(intro_msg)) / 2, "%s", intro_msg);
        attroff(COLOR_PAIR(2));
        refresh();
        sleep(1);
    }

    refresh();

}