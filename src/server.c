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
void *ptr_logs;                         // Shared memory for logger


sem_t *sem_logs_1, *sem_logs_2, *sem_logs_3;    // Semaphores for logger
sem_t *sem_wd_1, *sem_wd_2, *sem_wd_3;  // Semaphores for watchdog


// Sockets
int obstacles_socket;
int targets_socket;
int socket_fd;
int new_socket_fd, port_number, client_len ,pid;

char log_file[80];
char msg[1024];

int main(int argc, char *argv[])
{

    close_sockets();
    // clean_up();
    
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Set Configuration
    char program_type[MSG_LEN];
    char socket_data[MSG_LEN];
    read_args_from_file(CONFIG_PATH, program_type, socket_data);
    char host_name[MSG_LEN];
    int port_num;
    printf("Program type: %s\n", program_type);
    sprintf(msg, "Program type: %s", program_type);
    log_msg(log_file, SERVER, msg);

    parse_host_port(socket_data, host_name, &port_num);
    printf("Host name: %s\n", host_name);
    printf("Port number: %d\n", port_num);

    sprintf(msg, "Host name: %s\n", host_name);
    log_msg(log_file, SERVER, msg);

    sprintf(msg, "Port number: %d\n", port_num);
    log_msg(log_file, SERVER, msg);

    if (strcmp(program_type, "server") == 0)
    {
        exit(0);
    }

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);    
    sigaction (SIGUSR1, &sa, NULL);

    //////////////////////////////////////////////////////
    /* SHARED MEMORY and SEMAPHORES INITIALIZATION */
    /////////////////////////////////////////////////////

    // Shared memory and semaphores for WATCHDOG
    ptr_wd = create_shm(SHAREMEMORY_WD);

    sem_wd_1 = sem_open(SEMAPHORE_WD_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_1 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_wd_1 failed");
        exit(1);
    }
    sem_wd_2 = sem_open(SEMAPHORE_WD_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_2 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_wd_2 failed");
        exit(1);
    }
    sem_wd_3 = sem_open(SEMAPHORE_WD_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_3 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_wd_3 failed");
        exit(1);
    }

    // Shared memory for LOGS
    ptr_logs = create_shm(SHAREMEMORY_LOGS);

    sem_logs_1 = sem_open(SEMAPHORE_LOGS_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
    if (sem_logs_1 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_logs_1 failed");
        exit(1);
    }
    sem_logs_2 = sem_open(SEMAPHORE_LOGS_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
    if (sem_logs_2 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_logs_2 failed");
        exit(1);
    }
    sem_logs_3 = sem_open(SEMAPHORE_LOGS_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // this dude is locked
    if (sem_logs_3 == SEM_FAILED) 
    {
        log_err(log_file, SERVER, "sem_logs_3 failed");
        exit(1);
    }

    

    // when all shm are created publish your pid to WD
    publish_pid_to_wd(SERVER_SYM, getpid());


    //////////////////////////////////////////////////////
    /* SOCKET CREATION */
    /////////////////////////////////////////////////////

    int obstacles_socket = 0;
    int targets_socket = 0;
    int socket_fd = 0, new_socket_fd = 0, port_number, client_len, pid;
    struct sockaddr_in server_address, client_address;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        log_err(log_file, SERVER, "ERROR opening socket");
    }
    
    port_number = port_num; // Port number
    
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port_number);


    if (bind(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        log_err(log_file, SERVER, "ERROR on binding");
    }

    listen(socket_fd, 5); // Max 5 connections
    client_len = sizeof(client_address);


    //////////////////////////////////////////////////////
    /* HANDLE CLIENT CONNECTIONS */
    /////////////////////////////////////////////////////

    while(targets_socket == 0 || obstacles_socket == 0)
    {
        block_signal(SIGUSR1);
        new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_len);
        unblock_signal(SIGUSR1);
        
        if (new_socket_fd < 0)
        {
            log_err(log_file, SERVER, "ERROR on accept");
        }

        char socket_msg[MSG_LEN];
        read_and_echo(new_socket_fd, socket_msg);
        log_msg(log_file, "SOCKET", socket_msg);

        if (socket_msg[0] == 'O')
        {
            obstacles_socket = new_socket_fd;
        }
        else if (socket_msg[0] == 'T')
        {
            targets_socket = new_socket_fd;
        }
        else
        {
            close(new_socket_fd);
        }
        
    }

    char prev_drone_msg[MSG_LEN] = "";


    //Main loop
    while(1)
    {

        //////////////////////////////////////////////////////
        /* Handle pipe from key_manager.c */
        /////////////////////////////////////////////////////

        char km_msg[MSG_LEN];

        if (read_pipe_non_blocking(km_server[0], km_msg) == 1)
        {
            // Read acknowledgement
            printf("[PIPE] Received from key_manager.c: %s\n", km_msg);
            sprintf(msg, "[PIPE] Received from key_manager.c: %s\n", km_msg);
            log_msg(log_file, SERVER, msg);

            fflush(stdout);
            // STOP key pressed (P)
            if (km_msg[0] == 'S')
            {
                write_and_wait_echo(targets_socket, km_msg, sizeof(km_msg), log_file, SERVER);
                write_and_wait_echo(obstacles_socket, km_msg, sizeof(km_msg), log_file, SERVER);
                exit(0);
            }
            // Response
            char response_km_msg[MSG_LEN*2];
            sprintf(response_km_msg, "K:%s", km_msg);
            write_to_pipe(server_drone[1], response_km_msg);
            printf("[PIPE] Sent to drone.c: %s\n", response_km_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", response_km_msg);
            log_msg(log_file, SERVER, msg);
            fflush(stdout);
        }    

        //////////////////////////////////////////////////////
        /* Handle pipe from interface.c */
        /////////////////////////////////////////////////////


        char interface_msg[MSG_LEN];
        memset(interface_msg, 0 , MSG_LEN);

        if (read_pipe_non_blocking(interface_server[0], interface_msg) == 1)
        {
            // Read acknowledgement
            printf("[PIPE] Received from interface.c: %s\n", interface_msg);
            sprintf(msg, "[PIPE] Received from interface.c: %s\n", interface_msg);
            log_msg(log_file, SERVER, msg);
            fflush(stdout);
            // Response for drone.c
            write_to_pipe(server_drone[1], interface_msg);
            printf("[PIPE] Sent to drone.c: %s\n", interface_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", interface_msg);
            log_msg(log_file, SERVER, msg);
            fflush(stdout);

            if (interface_msg[0] == 'I' && interface_msg[1] == '2')
            {
                // Send to socket: Client Targets
                char sub_string_1[MSG_LEN];
                strcpy(sub_string_1, interface_msg + 3);
                write_and_wait_echo(targets_socket, sub_string_1, sizeof(sub_string_1), log_file, SERVER);
                // Send to socket: Client Obstacles
                char sub_string_2[MSG_LEN];
                strcpy(sub_string_2, interface_msg + 3);
                write_and_wait_echo(obstacles_socket, sub_string_2, sizeof(sub_string_2), log_file, SERVER);
                printf("SENT %s to obstacles.c and targets.c\n", sub_string_2);
                printf(msg, "SENT %s to obstacles.c and targets.c\n", sub_string_2);
                log_msg(log_file, SERVER, msg);
            }
            if (interface_msg[0] == 'G')
            {
                write_and_wait_echo(targets_socket, interface_msg, sizeof(interface_msg), log_file, SERVER);
            }
            
        }

        //////////////////////////////////////////////////////
        /* Handle pipe from drone.c */
        /////////////////////////////////////////////////////
        
        char drone_msg[MSG_LEN];
        if (read_pipe_non_blocking(drone_server[0], drone_msg) == 1)
        {
            write_to_pipe(server_interface[1], drone_msg);
            // Only send to drone when data has changed
            if (strcmp(drone_msg, prev_drone_msg) != 0) 
            {
                sprintf(msg, "[PIPE] Received from drone.c: %s\n", drone_msg);
                log_msg(log_file, SERVER, msg);
                fflush(stdout);
                // Update previous values
                strcpy(prev_drone_msg, drone_msg); 
                sprintf(msg, "[PIPE] Sent to interface.c: %s\n", drone_msg);
                log_msg(log_file, SERVER, msg);
                fflush(stdout);
            }
        }

        //////////////////////////////////////////////////////
        /* Handle socket from obstacles.c */
        /////////////////////////////////////////////////////

        char obstacles_msg[MSG_LEN];
        if (read_and_echo_non_blocking(obstacles_socket, obstacles_msg, log_file, SERVER) == 1)
        {
            // Send to interface.c
            write_to_pipe(server_interface[1],obstacles_msg);
            sprintf(msg, "[PIPE] Send to interface.c: %s\n", obstacles_msg);
            log_msg(log_file, SERVER, msg);
            fflush(stdout);
            // Send to drone.c
            write_to_pipe(server_drone[1], obstacles_msg);
            sprintf(msg, "[PIPE] Sent to drone.c: %s\n", obstacles_msg);
            log_msg(log_file, SERVER, msg);
            fflush(stdout);
        }        

        //////////////////////////////////////////////////////
        /* Handle socket from targets.c */
        /////////////////////////////////////////////////////

        char targets_msg[MSG_LEN];

        if (read_and_echo_non_blocking(targets_socket, targets_msg, log_file, SERVER) == 1)
        {
            // Send to interface.c
            write_to_pipe(server_interface[1],targets_msg);
            sprintf(msg, "[PIPE] Sent to interface.c: %s\n", targets_msg);
            log_msg(log_file, SERVER, msg);
        }

    }

    // Close and unlink the semaphores
    clean_up();
    return 0;

}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d %d %d %d %d %d %s", &km_server[0], &server_drone[1], 
    &interface_server[0], &drone_server[0], &server_interface[1],
    &server_obstacles[1], &obstacles_server[0], &server_targets[1],
    &targets_server[0], log_file);
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT\n");
        log_msg(log_file, SERVER, msg);
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
        log_err(log_file, SERVER, "shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE_SHM);

    /* memory map the shared memory object */
    void *shm_ptr = mmap(0, SIZE_SHM, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        log_err(log_file, SERVER, "Map Failed");
        exit(1);
    }

    close(shm_fd);
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

    // close all sockets
    close_sockets();

    sprintf(msg, "Clean up has been performed succesfully\n");
    log_msg(log_file, SERVER, msg);
}

void close_sockets()
{
    close(socket_fd);
    close(obstacles_socket);
    close(targets_socket);
    close(new_socket_fd);
}