#include "server.h"
#include "import.h"


// Pipes
int km_server[2];
int server_drone[2];
int interface_server[2];
int drone_server[2];
int server_interface[2];
int server_obstacles[2];
int obstacles_server[2];
int server_targets[2];
int targets_server[2];

// GLOBAL VARIABLES
// Attach to shared memory for key presses
void *ptr_wd;                           // Shared memory for WD
void *ptr_logs;                         // Shared memory ptr for logs
sem_t *sem_logs_1, *sem_logs_2, *sem_logs_3; // Semaphores for logger
sem_t *sem_wd_1, *sem_wd_2, *sem_wd_3;  // Semaphores for watchdog

int main(int argc, char *argv[])
{

    clean_up();
    
    get_args(argc, argv);

    /* Sigaction */
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);    
    sigaction (SIGUSR1, &sa, NULL);

    // Shared memory and semaphores for WATCHDOG
    shared_memory_handler('W');
    
    // Shared memory and semaphores for LOGS
    shared_memory_handler('L');

    // when all shm are created publish your pid to WD
    publish_pid_to_wd(SERVER_SYM, getpid());


    // To compare current and previous data
    char prev_drone_msg[MSG_LEN] = "";

    //Main loop
    while(1)
    {

        //////////////////////////////////////////////////////
        /* Handle pipe from key_manager.c */
        /////////////////////////////////////////////////////

        char km_msg[MSG_LEN];
        if(read_from_pipe(km_server[0], km_msg))
        {
            // Read acknowledgement
            printf("RECEIVED %s from key_manager.c\n", km_msg);
            fflush(stdout);
            // Response
            char response_km_msg[MSG_LEN*2];
            sprintf(response_km_msg, "K:%s", km_msg);
            write_to_pipe(server_drone[1], response_km_msg);
            printf("SENT %s to drone.c\n", response_km_msg);
            fflush(stdout);
        }


        //////////////////////////////////////////////////////
        /* Handle pipe from interface.c */
        /////////////////////////////////////////////////////
        
        char interface_msg[MSG_LEN];
        if(read_from_pipe(interface_server[0], interface_msg))
        {
            // Read acknowledgement
            printf("RECEIVED %s from interface.c\n", interface_msg);
            fflush(stdout);
            // Response for drone.c
            write_to_pipe(server_drone[1], interface_msg);
            printf("SENT %s to drone.c\n", interface_msg);
            fflush(stdout);
            // Response for obstacles.c and targets.c
            if(interface_msg[0] == 'I' && interface_msg[1] == '2')
            {
                write_to_pipe(server_obstacles[1],interface_msg);
                write_to_pipe(server_targets[1],interface_msg);
                printf("SENT %s to obstacles.c and targets.c\n", interface_msg);
            }
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from drone.c */
        /////////////////////////////////////////////////////

        char drone_msg[MSG_LEN];

        if(read_from_pipe(drone_server[0], drone_msg))
        {
            write_to_pipe(server_interface[1], drone_msg);
            if (strcmp(drone_msg, prev_drone_msg) != 0) 
            {
                printf("RECEIVED %s from drone.c\n", drone_msg);
                fflush(stdout);
                strcpy(prev_drone_msg, drone_msg); // Update previous values
                printf("SENT %s to interface.c\n", drone_msg);
                fflush(stdout);
            }
        }
        

        //////////////////////////////////////////////////////
        /* Handle pipe from obstacles.c */
        /////////////////////////////////////////////////////

        char obstacles_msg[MSG_LEN];
        if (read_from_pipe(obstacles_server[0], obstacles_msg))
        {
            // Read acknowledgement
            printf("RECEIVED %s from obstacles.c\n", obstacles_msg);
            fflush(stdout);
            // Send to interface.c
            write_to_pipe(server_interface[1],obstacles_msg);
            printf("SENT %s to interface.c\n", obstacles_msg);
            fflush(stdout);
            // Send to drone.c
            write_to_pipe(server_drone[1], obstacles_msg);
            printf("SENT %s to drone.c\n", obstacles_msg);
            fflush(stdout);
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from targets.c */
        /////////////////////////////////////////////////////

        char targets_msg[MSG_LEN];
        if (read_from_pipe(targets_server[0], targets_msg))
        {
            // Read acknowledgement
            printf("RECEIVED %s from targets.c\n", targets_msg);
            fflush(stdout);
            // Send to interface.c
            write_to_pipe(server_interface[1],targets_msg);
            printf("SENT %s to interface.c\n", targets_msg);
            fflush(stdout);
        }

    }

    // Close and unlink the semaphores
    clean_up();
    return 0;

}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d %d %d %d %d %d", &km_server[0], &server_drone[1], 
    &interface_server[0], &drone_server[0], &server_interface[1],
    &server_obstacles[1], &obstacles_server[0], &server_targets[1],
    &targets_server[0]);
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        printf("Caught SIGINT\n");
        clean_up();
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

void *create_shm(char *name)
{

    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE_SHM);

    /* memory map the shared memory object */
    void *shm_ptr = mmap(0, SIZE_SHM, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        perror("Map Failed");
        exit(1);
    }
    return shm_ptr;
}

void clean_up()
{
    // close all connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);

    // unlink semaphores
    sem_unlink(SEMAPHORE_WD_1);
    sem_unlink(SEMAPHORE_WD_2);
    sem_unlink(SEMAPHORE_WD_3);

    // unmap shared memory
    munmap(ptr_wd, SIZE_SHM);


    // unlink shared memories
    shm_unlink(SHAREMEMORY_WD);

    printf("Clean up has been performed succesfully\n");
}

void shared_memory_handler(char name)
{
    // Shared memory and semaphores for WATCHDOG
    if (name == 'W')
    {
        ptr_wd = create_shm(SHAREMEMORY_WD);
        sem_wd_1 = sem_open(SEMAPHORE_WD_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
        if (sem_wd_1 == SEM_FAILED) 
        {
            perror("sem_wd_1 failed");
            exit(1);
        }
        sem_wd_2 = sem_open(SEMAPHORE_WD_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
        if (sem_wd_2 == SEM_FAILED) 
        {
            perror("sem_wd_2 failed");
            exit(1);
        }
        sem_wd_3 = sem_open(SEMAPHORE_WD_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
        if (sem_wd_3 == SEM_FAILED) 
        {
            perror("sem_wd_2 failed");
            exit(1);
        }
    }

    // Shared memory for LOGS
    else if (name == 'L')
    {
        ptr_logs = create_shm(SHAREMEMORY_LOGS);
        sem_logs_1 = sem_open(SEMAPHORE_LOGS_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
        if (sem_logs_1 == SEM_FAILED) 
        {
            perror("sem_logs_1 failed");
            exit(1);
        }
        sem_logs_2 = sem_open(SEMAPHORE_LOGS_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
        if (sem_logs_2 == SEM_FAILED) 
        {
            perror("sem_logs_1 failed");
            exit(1);
        }
        sem_logs_3 = sem_open(SEMAPHORE_LOGS_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
        if (sem_logs_3 == SEM_FAILED) 
        {
            perror("sem_logs_1 failed");
            exit(1);
        }
    }
}