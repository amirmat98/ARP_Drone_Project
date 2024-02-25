#include "constants.h"
#include "import.h"

// Publisht the PID integer value to the watchdog
void publish_pid_to_wd(char process_symbol, pid_t pid)
{   
    int shm_wd_fd = shm_open(SHAREMEMORY_WD, O_RDWR, 0666);
    char *ptr_wd = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_wd_fd, 0);
    sem_t *sem_wd_1 = sem_open(SEMAPHORE_WD_1, 0);    // Protects shm_wd
    sem_t *sem_wd_2 = sem_open(SEMAPHORE_WD_2, 0);    // Allows WD to use SHM_WD
    sem_t *sem_wd_3 = sem_open(SEMAPHORE_WD_3, 0);    // WD allows processes to unlock sem_wd_1

    // Wait for signal from WD / other process to give your pid to WD
    sem_wait(sem_wd_1);
    snprintf(ptr_wd, sizeof(ptr_wd), "%i %i", process_symbol, pid);   
    // shm is set, signal WD so it can get the PID
    sem_post(sem_wd_2);
    // Wait for response from WD 
    sem_wait(sem_wd_3);
    // Allow other processes to do that. 
    sem_post(sem_wd_1);


    // PID's published, close your connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);

    // Detach from shared memorry
    munmap(ptr_wd,SIZE_SHM);
    // When all is done unlink from shm
}

// Writes message using the file descriptor provided.
void write_to_pipe(int pipe_des, char message[])
{
    ssize_t bytes_written = write(pipe_des, message, MSG_LEN);
    if (bytes_written == -1)
    {
        perror("Write went wrong");
        exit(1);
    }
}

// Writes the provided message into the logger.
void write_message_to_logger(int who, int type, char *msg)
{
    int shm_logs_fd = shm_open(SHAREMEMORY_LOGS, O_RDWR, 0666);
    void *ptr_logs = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_logs_fd, 0);

    sem_t *sem_logs_1 = sem_open(SEMAPHORE_LOGS_1, 0);
    sem_t *sem_logs_2 = sem_open(SEMAPHORE_LOGS_2, 0);
    sem_t *sem_logs_3 = sem_open(SEMAPHORE_LOGS_3, 0);

    sem_wait(sem_logs_1);

    // unsure if it will work
    sprintf(ptr_logs, "%i|%i|%s", who, type, msg);

    // Message is ready to be read
    sem_post(sem_logs_2);

    // wait for logger to finish writing
    sem_wait(sem_logs_3);

    // allow other processes to log their messages
    sem_post(sem_logs_1);

    // Detach from shared memorry
    munmap(ptr_logs,SIZE_SHM);
    close(shm_logs_fd);
    sem_close(sem_logs_1);
    sem_close(sem_logs_2);
    sem_close(sem_logs_3);
}

void error(char *msg)
{
    perror(msg);
    //exit(0);
}

// Reads a message from the pipe with select() system call.
int read_pipe_non_blocking(int pipe_des, char *message[])
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set read_pipe;
    FD_ZERO(&read_pipe);
    FD_SET(pipe_des, &read_pipe);

    char buffer[MSG_LEN];

    int ready = select(pipe_des + 1, &read_pipe, NULL, NULL, &timeout);
    if (ready == -1)
    {
        perror("Error in select");
    }

    if (ready > 0 && FD_ISSET(pipe_des, &read_pipe))
    {
        ssize_t bytes_read_pipe = read(pipe_des, buffer, MSG_LEN);
        if (bytes_read_pipe > 0 )
        {
            strcpy(message, buffer);
            return 1;
        }
        else
        {
            return 0;
        }
        
    }
}

// Reads a message from the socket, then does an echo.
void read_and_echo(int socket, char socket_msg[])
{
    int bytes_read, bytes_written;
    bzero(socket_msg, MSG_LEN);

    // Read from the socket

    bytes_read = read(socket, socket_msg, MSG_LEN - 1);
    if (bytes_read < 0) 
    {
        perror("ERROR reading from socket");
    }
    else if (bytes_read == 0)
    {
        return; // Connection closed
    }
    else if (socket_msg[0] == '\0')
    {
        return; // Empty message
    }

    printf("[SOCKET] Received: %s\n", socket_msg);
    
    // Echo data read into socket
    bytes_written = write(socket, socket_msg, bytes_read);
    if (bytes_written < 0)
    {
        perror("ERROR writing to socket");
    }
    printf("[SOCKET] Echo sent: %s\n", socket_msg);

}
// Reads a message from the socket, with select() system call, then does an echo.
int read_and_echo_non_blocking(int socket, char socket_msg[])
{
    int ready;
    int bytes_read, bytes_written;
    fd_set read_set;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Clear the buffer
    bzero(socket_msg, MSG_LEN);

    // Initialize the file descriptor set for monitoring.
    FD_ZERO(&read_set);
    FD_SET(socket, &read_set);

    // Use select() to wait for data to arrive.
    ready = select(socket + 1, &read_set, NULL, NULL, &timeout);
    if (ready < 0) 
    {
        perror("ERROR in select");
    }
    else if (ready == 0)
    {
        return 0;
        // No data available
    }

    // Data is available. Read from the socket
    bytes_read = read(socket, socket_msg, MSG_LEN - 1);
    if (bytes_read < 0) 
    {
        perror("ERROR reading from socket");
    }

    else if (bytes_read == 0)
    {
        return 0;
        // Connection closed
    }
    else if (socket_msg[0] == '\0')
    {
        return 0;
        // Empty message
    }

    // Print the received message
    printf("[SOCKET] Received: %s\n", socket_msg);

    // Echo the message back to the client.
    bytes_written = write(socket, socket_msg, bytes_read);
    if (bytes_written < 0)
    {
        perror("ERROR writing to socket");
    }
    else
    {
        printf("[SOCKET] Echo sent: %s\n", socket_msg);
        return 1;
    }
}
// Writes a message into the socket, then loops/waits until a valid echo is read.
void write_and_wait_echo(int socket, char socket_msg[], size_t msg_size)
{
    int ready;
    int bytes_read, bytes_written;

    bytes_written = write(socket, socket_msg, msg_size);
    if (bytes_written < 0)
    {
        perror("ERROR writing to socket");
    }
    printf("[SOCKET] Sent: %s\n", socket_msg);

    // Clear the buffer
    bzero(socket_msg, MSG_LEN);

    while(socket_msg[0] == '\0')
    {
        // Data is available for reading, so read from the socket
        bytes_read = read(socket, socket_msg, bytes_written);
        if (bytes_read < 0)
        {
            perror("ERROR reading from socket");
        }
        else if (bytes_read == 0)
        {
            printf("Connection closed\n");
            return;
        }
    }
    // Print the received message
    printf("[SOCKET] Echo received: %s\n", socket_msg);
}
