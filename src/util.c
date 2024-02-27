#include "import.h"

// Publisht the PID integer value to the watchdog
void publish_pid_to_wd(int process_symbol, pid_t pid)
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

void log_msg(char *file_path, char* who, char *message)
{
    // Open the file
    FILE *file_fd = fopen(file_path, "a");
    if (file_fd == NULL)
    {
        perror("Logfile open failed!");
        exit(1);
    }
    
    // Get the current time
    time_t currentTime;
    time(&currentTime);
    struct tm *localTime = localtime(&currentTime);
    int hours = localTime->tm_hour;
    int minutes = localTime->tm_min;
    int seconds = localTime->tm_sec;
    
    char time[80];
    char *eol = "\n";
    sprintf(time, "%02d:%02d:%02d", hours, minutes, seconds);
    
    fprintf(file_fd, "[INFO][%s] at [%s:] %s", who, time, message);
    fclose(file_fd); //close file at the end
    message = ""; // clear the buffer
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



// Reads a message from the socket, then does an echo.
void read_and_echo(int socket_des, char socket_msg[])
{
    int bytes_read, bytes_written;
    bzero(socket_msg, MSG_LEN);

    // Read from the socket

    bytes_read = read(socket_des, socket_msg, MSG_LEN - 1);
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

    // printf("[SOCKET] Received: %s\n", socket_msg);
    
    // Echo data read into socket
    bytes_written = write(socket_des, socket_msg, bytes_read);
    if (bytes_written < 0)
    {
        perror("ERROR writing to socket");
    }
    // printf("[SOCKET] Echo sent: %s\n", socket_msg);

}
// Reads a message from the socket, with select() system call, then does an echo.
int read_and_echo_non_blocking(int socket_des, char socket_msg[], char* log_file,char *log_who)
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
    FD_SET(socket_des, &read_set);

    // Use select() to wait for data to arrive.
    ready = select(socket_des + 1, &read_set, NULL, NULL, &timeout);
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
    bytes_read = read(socket_des, socket_msg, MSG_LEN - 1);
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
    // printf("[SOCKET] Received: %s\n", socket_msg);
    char msg_logs[1024];
    sprintf(msg_logs, "[SOCKET] Received: %s\n", socket_msg);
    log_msg(log_file, log_who, msg_logs);

    // Echo the message back to the client.
    bytes_written = write(socket_des, socket_msg, bytes_read);
    if (bytes_written < 0)
    {
        perror("ERROR writing to socket");
    }
    else
    {
        sprintf(msg_logs,"[SOCKET] Echo sent: %s\n", socket_msg);
        log_msg(log_file, log_who, msg_logs);
        return 1;
    }
}
// Writes a message into the socket, then loops/waits until a valid echo is read.
void write_and_wait_echo(int socket_des, char socket_msg[], size_t msg_size, char *log_file, char* log_who)
{
    int correct_echo = 0;
    int bytes_read, bytes_written;
    char response_msg[MSG_LEN];

    char msg[1024];

    while(correct_echo == 0)
    {
        bytes_written = write(socket_des, socket_msg, msg_size);
        if (bytes_written < 0)
        {
            perror("ERROR writing to socket");
        }
        sprintf(msg, "[SOCKET] Sent: %s\n", socket_msg);
        log_msg(log_file, log_who, msg);

        // Clear the buffer
        bzero(response_msg, MSG_LEN);

        while(response_msg[0] == '\0')
        {
            // Data is available for reading, so read from the socket
            bytes_read = read(socket_des, response_msg, bytes_written);
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

        if (strcmp(socket_msg, response_msg) == 0)
        {
            // Print the received message
            sprintfmsg, ("[SOCKET] Echo received: %s\n", response_msg);
            log_msg(log_file, log_who, msg);
            correct_echo = 1;
        }
    }
}

// Reads a message from the pipe with select() system call.
int read_pipe_non_blocking(int pipe_des, char message[])
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
        if (bytes_read_pipe > 0)
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

// Reads the first line of uncommented text from a file
void read_args_from_file(const char *file_name, char *type, char *data)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL) 
    {
        fprintf(stderr, "Error opening file %s\n", file_name);
        // exit(EXIT_FAILURE);
    }

    // Read lines until finding the first non-comment line
    char line[MSG_LEN];
    while (fgets(line, sizeof(line), file) != NULL) 
    {
        // Skip lines starting with '#' (comments)
        if (line[0] != '#') 
        {
            // Tokenize the line to extract type and data
            char *token = strtok(line, " \t\n");
            if (token != NULL) 
            {
                strncpy(type, token, MSG_LEN);
                token = strtok(NULL, " \t\n");
                if (token != NULL) 
                {
                    strncpy(data, token, MSG_LEN);
                    fclose(file);
                    return;
                }
            }
        }
    }
}

// Obtains both the host name and the port number from a formatted string
void parse_host_port(const char *str, char *host, int *port)
{
    char *colon = strchr(str, ':');
    if (colon == NULL)
    {
        fprintf(stderr, "Invalid input: No colon found\n");
        exit(EXIT_FAILURE);
    }

    int len = colon - str;
    strncpy(host, str, len);
    host[len] = '\0';

    *port = atoi(colon + 1);
}