#include "interface.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>


// Serverless pipes
int key_pressing[2];
int lowest_target_fd[2];

// Pipes working with the server
int interface_server[2];
int server_interface[2];

int main(int argc, char *argv[])
{
    // Get the file descriptors for the pipes from the arguments
    get_args(argc, argv);

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);  
    sigaction (SIGUSR1, &sa, NULL);
    publish_pid_to_wd(WINDOW_SYM, getpid());

    //----------------------------------------------------------------------------------------//

    // INITIALIZATION AND EXECUTION OF NCURSES FUNCTIONS
    initscr(); // Initialize
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    start_color(); // Initialize color for drawing drone
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone will be of color blue
    noecho(); // Disable echoing: no key character will be shown when pressed.

    /* SET INITIAL DRONE POSITION */
    // Obtain the screen dimensions
    int screen_size_x;
    int screen_size_y;
    int prev_screen_size_y = 0;
    int prev_screen_size_x = 0;
    getmaxyx(stdscr, screen_size_y, screen_size_x);
     // Initial drone position will be the middle of the screen.
    int drone_x = screen_size_x / 2;
    int drone_y = screen_size_y / 2;

    // Write the initial position and screen size data into the server
    char initial_msg[MSG_LEN];
    sprintf(initial_msg, "I1:%d,%d,%d,%d", drone_x, drone_y, screen_size_x, screen_size_y);
    write_to_pipe(interface_server[1], initial_msg);


    /* Useful variables creation*/
    // To compare current and previous data
    char prev_lowest_target[] = "";
    int obtained_targets = 0;
    int obtained_obstacles = 0;
    // For game score calculation
    int score = 0;
    char score_msg[MSG_LEN];
    int counter = 0;
    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Targets and obstacles
    // char targets_msg[] = "T[8]140,23|105,5|62,4|38,6|50,16|6,25|89,34|149,11";
    Targets targets[80];
    int number_targets;
    // parse_target_message(targets_msg, targets, &number_targets);

    Obstacles obstacles[80];
    int number_obstacles;


    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: SEND SCREEN DIMENSIONS TO SERVER */
        /////////////////////////////////////////////////////

        getmaxyx(stdscr, screen_size_y, screen_size_x);
        /*SEND x,y  to server ONLY when screen size changes*/
        if (screen_size_x != prev_screen_size_x || screen_size_y != prev_screen_size_y)
        {
            // Update previous values
            prev_screen_size_x = screen_size_x;
            prev_screen_size_y = screen_size_y;
            // Send data
            char screen_msg[MSG_LEN];
            sprintf(screen_msg, "I2:%d,%d", screen_size_x, screen_size_y);
            write_to_pipe(interface_server[1], screen_msg);
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_interface[0], &read_fds);
        
        char server_msg[MSG_LEN];

        int ready;
        do 
        {
            ready = select(server_interface[0] + 1, &read_fds, NULL, NULL, NULL);
            if (ready == -1) 
            {
                perror("Error in select");
            }
        } while (ready == -1 && errno == EINTR);

        ssize_t bytes_read_drone = read(server_interface[0], server_msg, MSG_LEN);
        if (bytes_read_drone > 0) 
        {
            if (server_msg[0] == 'D')
            {
                sscanf(server_msg, "D:%d,%d", &drone_x, &drone_y);
            }
            else if (server_msg[0] == 'O')
            {
                parseObstaclesMsg(server_msg, obstacles, &numObstacles);
                obtained_obstacles = 1;
            }
            else if (server_msg[0] == 'T')
            {
                parse_target_message(server_msg, targets, &number_targets);
                obtained_targets = 1;
            }
        }

        
        if (obtained_targets == 0 && obtained_obstacles == 0)
        {
            continue;
        }

        //////////////////////////////////////////////////////
        /* SECTION 3: DATA ANALYSIS & GAME SCORE CALCULATION*/
        /////////////////////////////////////////////////////

        /* UPDATE THE TARGETS ONCE THE DRONE REACHES THE CURRENT LOWEST NUMBER */
        // Find the index of the target with the lowest ID
        int lowest_index = find_lowest_ID(targets, number_targets);
        // Obtain the coordinates of that target
        char lowest_target[20];
        sprintf(lowest_target, "%d,%d", targets[lowest_index].x, targets[lowest_target].y);
        // Send to drone w/ serverless pipe lowest_target
        write_to_pipe(lowest_target_fd[1], lowest_target);

        if (targets[lowest_index].x == drone_x && targets[lowest_index].y == drone_y) 
        {
            // Update score and counter (reset timer)
            if (counter > 400) 
            {  // 400 * 50ms = 20,000ms = 20 seconds 
                score += 2;
            }
            else if (counter > 200) 
            {  // 200 * 50ms = 10,000ms = 10 seconds 
                score += 6;
            }
            else if (counter > 100) 
            {  // 100 * 50ms = 5000ms = 5 seconds 
                score += 8;
            }
            else 
            {  // Les than 5 seconds
                score += 10;
            }
            counter = 0;
            // Remove the target with the lowest ID
            remove_target(targets, &number_targets, lowest_index);
        }

        // Check if the drone has crashed into an obstacle
        if (check_collision_drone_obstacle(obstacles, number_obstacles, drone_x, drone_y))
        {
            // Update score
            score -= 5;
        }

        //////////////////////////////////////////////////////
        /* SECTION 4: DRAW THE WINDOW & OBTAIN INPUT*/
        /////////////////////////////////////////////////////

        // Create a string for the player's current score
        sprintf(score_msg, "Your current score: %d", score);

        // Draws the window with the updated information of the terminal size, drone, targets, obstacles and score.
        draw_window(drone_x, drone_y, targets, number_targets, obstacles, number_obstacles, score_msg);

        /* HANDLE THE KEY PRESSED BY THE USER */
        int ch;
        if ((ch = getch()) != ERR)
        {
            // Write char to the pipe
            char msg[MSG_LEN];
            sprintf(msg, "%c", ch);
            // Send it directly to key_manager.c
            write_to_pipe(key_pressing[1], msg);
        }

        flushinp(); // Clear the input buffer

        // Counter/Timer linked to score calculations
        counter++;
        usleep(50000);


    }

    // Clean up and finish up resources taken by ncurses
    endwin(); 

    return 0;
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d", &key_pressing[1], &server_interface[0], 
    &interface_server[1], &lowest_target_fd[1]);
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // sleep(2);
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        //TODO REFRESH SCREEN
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}

void draw_window(int drone_x, int drone_y, Targets *targets, int number_targets, 
                    Obstacles *obstacles, int number_obstacles, const char *score_msg)
{
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);

    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);
    refresh();

    // Print a title in the top center part of the window
    mvprintw(0, (max_x - 11) / 2, "%s", score_msg);

    // Draw a plus sign to represent the drone
    mvaddch(drone_x, drone_y, '+' | COLOR_PAIR(1));
    refresh();

    // Draw obstacles on the board
    for (int i = 0; i < number_obstacles, i++)
    {
        // Assuming 'O' represents obstacles
        mvaddch(obstacles[i].y, obstacles[i].x, '0');
        refresh();
    }
    
    // Draw targets on the board
    for (int i = 0; i < number_targets; i++)
    {
        mvaddch(targets[i].y, targets[i].x, targets[i].ID + '0');
        refresh();
    }
    
}


// Function to find the index of the target with the lowest ID
int find_lowest_ID(Targets *targets, int number_targets)
{
    int lowest_ID = targets[0].ID;
    int lowest_index = 0;
    for (int i = 1; i<number_targets; i++)
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
    sscanf(obstacles_msg, "0[%d]", &total_obstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *number_obstacles = 0;

    while (token != NULL && *number_obstacles < total_obstacles)
    {
        sscanf(token, "%d , %d", &obstacles[*number_obstacles].x, &obstacles[*number_obstacles].y);
        obstacles[*number_obstacles].total = *number_obstacles + 1;

        token = strtok(NULL, "|");
        (*number_obstacles)++;
    }
}

// Function to extract the coordinates from the targets_msg string into the struct Targets.
void parse_target_message(char *targets_msg, Targets *targets, int *number_targets)
{
    char *token = strtok(targets_msg + 4, "|");
    *number_targets = 0;

    while (token != NULL)
    {
        sscanf (token, "%d , %d", &targets[*number_targets].x, &targets[*number_targets].y );
        targets[*number_targets].ID = *number_targets + 1;

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


/*
// Function called from the drone has reached the correct target.
void update_score(int *score, int *counter)
{
    if (*counter > 400)
    {
        // 400 * 50ms = 20,000ms = 20 seconds 
        *score += 2;
    }
    else if (*counter > 200)
    {
        // 200 * 50ms = 10,000ms = 10 seconds 
        *score += 6;
    }
    else if (*counter > 100)
    {
        // 100 * 50ms = 5000ms = 5 seconds 
        *score += 8;
    }
    else if (*counter <= 100)
    {
        // Les than 5 seconds
        *score += 10;
    }
    *counter = 0;
}
*/

int decypher_message(char server_msg[]) 
{
    if (server_msg[0] == 'D') 
    {
        return 1;
    } 
    else if (server_msg[0] == 'T') 
    {
        return 2;
    } else if (server_msg[0] == 'O') 
    {
        return 3;
    }
    else 
    {
        return 0;
    }
}