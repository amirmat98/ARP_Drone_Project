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
#define SEMAPHORE_LOGS_1 "/my_semaphore_logs_1"
#define SEMAPHORE_LOGS_2 "/my_semaphore_logs_2"
#define SEMAPHORE_LOGS_3 "/my_semaphore_logs_3"

// Pipes Constants
#define MSG_LEN 240

// Length for logfiles
#define LOG_LEN 256

// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

#define SLEEP_DELAY 100000


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

// struct of obstacles
typedef struct {
    int total;
    int x;
    int y;
} Obstacles;

// struct of Targets
typedef struct {
    int ID;
    int x;
    int y;
} Targets;



#endif // CONSTANTS_H