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

#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>

/* Global variables */
// Attach to shared memory for key presses
// int shm_key_fd;         // File descriptor for key shm
int shm_pos_fd;         // File descriptor for drone pos shm
// int *ptr_key;           // Shared memory for Key pressing
char *ptr_pos;          // Shared memory for Drone Position
// sem_t *sem_key;         // Semaphore for key presses
sem_t *sem_pos;         // Semaphore for drone positions

/*
//WatchDog
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_pos);

        printf("Succesfully closed all semaphores\n");
        sleep(10);
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
*/

// Pipes
int key_pressing[2];

int main(int argc, char *argv[])
{
    get_args(argc, argv);


    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);  
    sigaction (SIGUSR1, &sa, NULL);

    publish_pid_to_wd(WINDOW_SYM, getpid());

    // Shared memory for KEY PRESSING
    // shm_key_fd = shm_open(SHAREMEMORY_KEY, O_RDWR, 0666);
    // ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_key_fd, 0);


    // Shared memory for DRONE POSITION
    shm_pos_fd = shm_open(SHAREMEMORY_POSITION, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    // sem_key = sem_open(SEMAPHORE_KEY, 0);
    sem_pos = sem_open(SEMAPHORE_POSITION, 0);
    //----------------------------------------------------------------------------------------//

    /* INITIALIZATION AND EXECUTION OF NCURSES FUNCTIONS */
    initscr(); // Initialize
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    start_color(); // Initialize color for drawing drone
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone will be of color blue
    noecho(); // Disable echoing: no key character will be shown when pressed.

    // Set the initial drone position (middle of the window)
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int drone_x = max_x / 2;
    int drone_y = max_y / 2;

    // Write initial drone position in its corresponding shared memory
    sprintf(ptr_pos, "%d,%d,%d,%d", drone_x, drone_y, max_x, max_y);


    // TARGETS: Should be obtained from a pipe from server.c
    char targets_msg[] = "T[8]140,23|105,5|62,4|38,6|50,16|6,25|89,34|149,11";
    Targets targets[30];
    int number_targets;
    parse_target_message(targets_msg, targets, &number_targets);

    // For game score calculation
    int score = 0;
    int counter = 0;


    while (1)
    {
        sem_wait(sem_pos); // Wait for the semaphore to be signaled from drone.c process
        
        // OBSTACLES: Should be obtained from a pipe from server.c
        char obstacles_msg[] = "O[7]35,11|100,5|16,30|88,7|130,40|53,15|60,10";
        Obstacles obstacles[30];
        int number_obstacles;
        parse_obstacles_message(obstacles_msg, obstacles, &number_obstacles);

        
        // Obtain the position values from shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &drone_x, &drone_y, &max_x, &max_y);

        // Create the window
        //create_window(max_x, max_y);
        // Draw the drone on the screen based on the obtained positions.
        //draw_drone(drone_x, drone_y);

        // UPDATE THE TARGETS ONCE THE DRONE REACHES THE CURRENT LOWEST NUMBER 
        //int number_targets = sizeof (targets) / sizeof (targets[0]);
        // Find the index of the target with the lowest ID
        int lowest_index = find_lowest_ID(targets, number_targets);

        // Obtain the coordinates of that target
        char lowest_target[20];
        sprintf(lowest_target, "%d , %d", targets[lowest_index].x, targets[lowest_index].y);




        // Check if the coordinates of the lowest ID target match the drone coordinates
        if (targets[lowest_index].x == drone_x && targets[lowest_index].y == drone_y)
        {
            // Update score and counter (reset timer)
            // update_score(&counter, &score);



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




            remove_target(targets, &number_targets, lowest_index);
        }

        // Check if the drone has crashed into an obstacle
        if (check_collision_drone_obstacle(obstacles, number_obstacles, drone_x, drone_y))
        {
            // update score
            score -= 5;
        }

        // Create a string for the player's current score
        char score_msg[50];
        sprintf(score_msg, "Your current scroe: %d", score);

        // Draws the window with the updated information of the terminal size, drone, targets and obstacles
        draw_window(drone_x, drone_y, targets, number_targets, obstacles, number_obstacles, score_msg);

        // Call the function that obtains the key that has been pressed.
        // handle_input(ptr_key, sem_key);

        // sem_post(sem_key);    // unlocks to allow keyboard manager

        /* HANDLE THE KEY PRESSED BY THE USER */
        int ch;
        if ((ch == getch()) != ERR)
        {
            // Write char to the pipe
            char msg[MSG_LEN];
            sprintf(msg, "%c", ch);
            write_to_pipe(key_pressing[1], msg);
        }
        flushinp(); // Clear the input buffer

        // Timer that is linked to score calculations, and ncurses window stability.
        counter++;
        usleep(50000);




        // usleep(10000);
    }

    // Clean up and finish up resources taken by ncurses
    endwin(); 
    // close shared memories
    // close(shm_key_fd);
    close(shm_pos_fd);

    // Close and unlink the semaphore
    // sem_close(sem_key);
    sem_close(sem_pos);

    return 0;
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d", &key_pressing[1]);
}
void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        // sem_close(sem_key);
        sem_close(sem_pos);

        printf("Succesfully closed all semaphores\n");
        sleep(2);
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
    }
    refresh();

    // Draw targets on the board
    for (int i = 0; i < number_targets; i++)
    {
        mvaddch(targets[i].y, targets[i].x, targets[i].ID + '0');
    }
    

    // Refresh the screen to apply changes
    refresh();
}


/*
void handle_input(int *shared_key, sem_t *semaphore)
{
    int temp_char;
    if ((temp_char = getch()) != ERR)
    {
        // write char to the pipe
        char msg[MSG_LEN];
        sprintf(msg, "%c", temp_char);
        write_to_pipe(key_pressing[1], msg);

        // Store the pressed key in shared memory
        *shared_key = temp_char;
    }
    flushinp(); // Clear the input buffer
}
*/

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