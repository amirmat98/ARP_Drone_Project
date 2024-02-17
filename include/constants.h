#ifndef CONSTANTS_H
#define CONSTANTS_H

// Shared memory for Watchdog's pid
#define SHAREMEMORY_WD "/wd_pid_1"
#define SEMAPHORE_WD_1 "/sem_wd_1"
#define SEMAPHORE_WD_2 "/sem_wd_2"
#define SEMAPHORE_WD_3 "/sem_wd_3"

// Shared memory key for key presses
#define SHAREMEMORY_KEY "/shared_keyboard"
#define SEMAPHORE_KEY "/my_semaphore_1"

// Shared memory key for drone positions
#define SHAREMEMORY_POSITION "/shared_drone_pos"
#define SEMAPHORE_POSITION "/my_semaphore_2"

// Shared memory key for drone control (actions).
#define SHAREMEMORY_ACTION "/shared_control"
#define SEMAPHORE_ACTION "/my_semaphore_3"

// Shared memory key for LOGS
#define SHAREMEMORY_LOGS "/shared_logs"
#define SEMAPHORE_LOGS "/my_semaphore_4"

// Pipes Constants
#define MSG_LEN 240

// Length for logfiles
#define LOG_LEN 256

// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0


// SYMBOLS FOR WATCHDOG & LOGGER
#define SERVER_SYM 1
#define WINDOW_SYM 2
#define KM_SYM 3
#define DRONE_SYM 4
#define LOGGER_SYM 5
#define WD_SYM 6

// SYMBOLS FOR LOG TYPES
#define INFO 1
#define WARN 2
#define ERROR 3

#define SIZE_SHM 4096 // General size of Shared memory (in bytes)
#define FLOAT_TOLERANCE 0.0044

#define SLEEP_DRONE 100000  // To let the interface.c process execute first write the initial positions.

// Drone Constants
#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define D_T 0.1 // Time interval in seconds
#define F_MAX 30.0 // Maximal force that can be acted on the drone's motors
#define EXT_FORCE_MAX 40.0 // Maximum external force on any axis.
const double coefficient = 400.0; // This was obtained by trial-error
double min_distance = 2.0;
double start_distance = 5.0; 

#define THRESHOLD 50 // Threshold for counters



// struct of obstacles
struct Obstacles
{
    int total;
    int x;
    int y;
};

// struct of Targets
struct Targets
{
    int ID;
    int x;
    int y;
};








#endif // CONSTANTS_H