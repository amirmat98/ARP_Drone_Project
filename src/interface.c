#include "interface.h"
#include "constants.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
int shm_key_fd;         // File descriptor for key shm
int shm_pos_fd;         // File descriptor for drone pos shm
int *ptr_key;           // Shared memory for Key pressing
char *ptr_pos;          // Shared memory for Drone Position
sem_t *sem_key;         // Semaphore for key presses
sem_t *sem_pos;         // Semaphore for drone positions

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

int main()
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);  
    sigaction (SIGUSR1, &sa, NULL);

    publish_pid_to_wd(WINDOW_SYM, getpid());

    // Shared memory for KEY PRESSING
    int shm_key_fd = shm_open(SHAREMEMORY_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_key_fd, 0);


    // Shared memory for DRONE POSITION
    int shm_pos_fd = shm_open(SHAREMEMORY_POSITION, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    sem_key = sem_open(SEMAPHORE_KEY, 0);
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


    while (1)
    {
        sem_wait(sem_pos); // Wait for the semaphore to be signaled from drone.c process
        // Obtain the position values from shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &drone_x, &drone_y, &max_x, &max_y);
        // Create the window
        create_window(max_x, max_y);
        // Draw the drone on the screen based on the obtained positions.
        draw_drone(drone_x, drone_y);
        // Call the function that obtains the key that has been pressed.
        handle_input(ptr_key, sem_key);

        sem_post(sem_key);    // unlocks to allow keyboard manager
    }

    // Clean up and finish up resources taken by ncurses
    endwin(); 
    // close shared memories
    close(shm_key_fd);
    close(shm_pos_fd);

    // Close and unlink the semaphore
    sem_close(sem_key);
    sem_close(sem_pos);

    return 0;
}

void create_window(int max_x, int max_y)
{
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int new_max_x, new_max_y;
    getmaxyx(stdscr, new_max_y, new_max_x);

    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);

    // Print a title in the top center part of the window
    mvprintw(0, (new_max_x - 11) / 2, "Drone Control");

    // Refresh the screen to apply changes
    refresh();
}

void draw_drone(int drone_x, int drone_y)
{

    // Draw a plus sign to represent the drone
    mvaddch(drone_y, drone_x, '+' | COLOR_PAIR(1));

    refresh();
}

void handle_input(int *shared_key, sem_t *semaphore)
{
    int ch;
    if ((ch = getch()) != ERR)
    {
        // Store the pressed key in shared memory
        *shared_key = ch;
    }
    flushinp(); // Clear the input buffer
}
