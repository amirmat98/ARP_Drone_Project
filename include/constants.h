#ifndef CONSTANTS_H
#define CONSTANTS_H

// Path to config file
#define CONFIG_PATH "./configuration.txt"

// Pipes Constants
#define MSG_LEN 256

// Socket Constants
#define MSG_BUF_LEN 1024

// Length for logfiles
#define LOG_LEN 256
#define LOG_FILE_LEN 80

// Shared memory for Watchdog's pid
#define SHAREMEMORY_WD "/wd_pid_1"
#define SEMAPHORE_WD_1 "/sem_wd_1"
#define SEMAPHORE_WD_2 "/sem_wd_2"
#define SEMAPHORE_WD_3 "/sem_wd_3"

// Shared memory key for LOGS
#define SHAREMEMORY_LOGS "/shared_logs"
#define SEMAPHORE_LOGS_1 "/my_semaphore_logs_1"
#define SEMAPHORE_LOGS_2 "/my_semaphore_logs_2"
#define SEMAPHORE_LOGS_3 "/my_semaphore_logs_3"


// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

// Define Genral Delay
#define GENERAL_DELAY 1000000

// SYMBOLS FOR WATCHDOG
#define SERVER_SYM 1
#define WINDOW_SYM 2
#define KM_SYM 3
#define DRONE_SYM 4
#define LOGGER_SYM 5
#define WD_SYM 6
#define OBSTACLES_SYM 7
#define TARGETS_SYM 8

// STRINGS FOR LOGS
#define SERVER "SERVER"
#define INTERFACE "INTERFACE"
#define KM "KEY MANAGER"
#define DRONE "DRONE"
#define WD "WATCHDOG"
#define OBS "OBSTACLES"
#define TARGETS "TARGETS"

// SYMBOLS FOR LOG TYPES
#define INFO 1
#define WARN 2
#define ERROR 3

#define SIZE_SHM 4096 // General size of Shared memory (in bytes)

// Struct to save obstacle's data
typedef struct {
    int total;
    int x;
    int y;
} Obstacles;

// Struct to save target's data
typedef struct {
    int ID;
    int x;
    int y;
} Targets;


#endif // CONSTANTS_H