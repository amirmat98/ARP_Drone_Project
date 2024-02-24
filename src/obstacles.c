#include "obstacles.h"
#include "util.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>

#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>


// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];


int main(int argc, char *argv[])
{
    sleep(1);
    get_args(argc, argv);

    // Seed the random number generator
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];
    int number_obstacles = 0;
    char obstacles_msg[MSG_LEN]; // Adjust the size based on your requirements

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // To compare previous values
    bool obtained_dimensions = false;


    // Variables
    int screen_size_x; 
    int screen_size_y;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int socket_fd;
    int prot_number;
    int n;

    struct sockaddr_in server_address;
    struct hostent *server_host;

    prot_number = 2000; // Port number
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Error opening socket");
        // exit(1);
    }
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Error, no host named 'localhost'.\n");
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(prot_number);
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error connecting");
        // exit(1);
    }

    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char msg[MSG_LEN]; // Adjust the size based on your requirements
    strcpy(msg, "OI");
    write_message_and_wait_for_echo(socket_fd, msg);

    while (1)
    {
        //////////////////////////////////////////////////////
        /* Step 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        // Initialize a set to track read events.
        fd_set active_read_set;
        // Clear all entries from the set.
        FD_ZERO(&active_read_set);
        // Add the first server obstacle's file descriptor to the set.
        FD_SET(server_obstacles[0], &active_read_set);

        // Buffer to store the server's message.
        char buffer_server[MSG_LEN];

        // Wait for any of the file descriptors to be ready for reading, or for a timeout.
        int num_fds_ready = select(server_obstacles[0] + 1, &active_read_set, NULL, NULL, &timeout);
        if (num_fds_ready == -1) 
        {
            // Log an error if select fails.
            perror("Select encountered an error");
        }

        // Check if there's data to read from the first server obstacle.
        if (num_fds_ready > 0 && FD_ISSET(server_obstacles[0], &active_read_set)) 
        {
            // Read data into the buffer.
            ssize_t count_bytes = read(server_obstacles[0], buffer_server, MSG_LEN);
            if (count_bytes > 0) 
            {
                // Process the message received from the server.
                receive_message_from_server(buffer_server, &screen_size_x, &screen_size_y);
                printf("Screen size: %d x %d\n", screen_size_x, screen_size_y);
                printf("-----------------------------------\n");
                // Indicate that the screen dimensions have been received.
                obtained_dimensions = true;
            }
        }
        // Continue the loop if the screen dimensions have not been obtained yet.
        if(!obtained_dimensions)
        {
            continue;
        }

        //////////////////////////////////////////////////////
        /* Step 2: OBSTACLES GENERATION & SEND DATA */
        /////////////////////////////////////////////////////


        if (number_obstacles < MAX_OBSTACLES) 
        {
            obstacles[number_obstacles] = generate_obstacle(screen_size_x, screen_size_y);
            number_obstacles++;
        }


        obstacles_msg[0] = '\0'; // Clear the string

        print_obstacles(obstacles, number_obstacles, obstacles_msg);

        sleep(WAIT_TIME);

        // Check if any obstacle has exceeded its spawn time
        check_obstacles_spawn_time(obstacles, number_obstacles, screen_size_x, screen_size_y);

        // SEND DATA TO SERVER
        write_to_pipe(obstacles_server[1],obstacles_msg);

    }

    return 0;
    
}

void print_obstacles(Obstacle obstacles[], int number_obstacles, char obstacles_msg[])
{
    // Append the letter O and the total number of obstacles to obstacles_msg
    sprintf(obstacles_msg, "O[%d]", number_obstacles);

    for (int i = 0; i < number_obstacles; i++)
    {
        // Append obstacle information to obstacles_msg
        sprintf(obstacles_msg + strlen(obstacles_msg), "%.3f,%.3f", (float)obstacles[i].x, (float)obstacles[i].y);
        // Add a separator if there are more obstacles
        if (i < number_obstacles - 1)
        {
            sprintf(obstacles_msg + strlen(obstacles_msg), "|");
        }
    }
    printf("%s\n", obstacles_msg);
    fflush(stdout);
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_obstacles[0], &obstacles_server[1]);
}

void receive_message_from_server(char *message, int *x, int *y)
{
    printf("Obtained from server: %s\n", message);
    float temp_scx, temp_scy;
    sscanf(message, "I2:%f,%f", &temp_scx, &temp_scy);
    *x = (int)temp_scx;
    *y = (int)temp_scy;
    // printf("Screen size: %d x %d\n", *x, *y);
    fflush(stdout);
}

Obstacle generate_obstacle(int x, int y)
{
    Obstacle obstacle;
    obstacle.x = (int)(0.05 * x) + (rand() % (int)(0.95 * x));
    obstacle.y = (int)(0.05 * y) + (rand() % (int)(0.95 * y));
    obstacle.spawn_time = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
    return obstacle;
}

void check_obstacles_spawn_time(Obstacle obstacles[], int number_obstacles, int x, int y)
{
    time_t currentTime = time(NULL);
    for (int i = 0; i < number_obstacles; ++i)
    {
        if (obstacles[i].spawn_time <= currentTime)
        {
            // Replace the obstacle with a new one
            obstacles[i] = generate_obstacle(x, y);
        }
    }
}

char* read_and_echo(int socket)
{
    int n1, n2;
    static char buffer[MSG_LEN];
    bzero(buffer, MSG_LEN);

    // Read the message from the socket.
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error reading from socket");
        // exit(1);
    }
    printf("Message from server: %s\n", buffer);

    // Echo data back to the socket.
    n2 = write(socket, buffer, n1);
    if (n2 < 0)
    {
        perror("Error writing to socket");
        // exit(1);
    }

    return buffer;

}
char* read_and_echo_non_blocking(int socket)
{
    static char buffer[MSG_LEN];
    fd_set active_read_set;
    struct timeval timeout;
    int n1, n2;
    int ready;

    // clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to monitor.
    FD_ZERO(&active_read_set);
    FD_SET(socket, &active_read_set);

    // Set the timeout for select.
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Use select to wait for data to become available.
    ready = select(socket + 1, &active_read_set, NULL, NULL, &timeout);
    if (ready < 0)
    {
        perror("Error in select");
        // exit(1);
    }
    else if (ready == 0)
    {
        printf("No data available\n");
        return NULL;
    }

    // Data is available, read it from socket.
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error reading from socket");
        // exit(1);
    }
    else if (n1 == 0)
    {
        printf("Connection closed\n");
        return NULL;
    }

    // Pritn the message from the socket.
    printf("Message from server: %s\n", buffer);

    // Echo data back to the socket.
    n2 = write(socket, buffer, n1);
    if (n2 < 0)
    {
        perror("Error writing to socket");
        // exit(1);
    }

    return buffer;

}

void write_message_and_wait_for_echo(int socket, char *message)
{
    static char buffer[MSG_LEN];
    fd_set read_set;
    struct timeval timeout;
    int n1, n2;
    int ready;

    n2 = write(socket, message, sizeof(message));
    if (n1 < 0)
    {
        perror("Error writing to socket");
        // exit(1);
    }
    printf("Message sent to server: %s\n", message);

    // Clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to monitor.
    FD_ZERO(&read_set);
    FD_SET(socket, &read_set);

    // Set the timeout for select.
    timeout.tv_sec = 0;
    timeout.tv_usec = 300000;

    // Use select to wait for data to become available.
    ready = select(socket + 1, &read_set, NULL, NULL, &timeout);
    if (ready < 0)
    {
        perror("Error in select");
        // exit(1);
    }
    else if (ready == 0)
    {
        printf("Timeout from server. Connection lost!");
        return;
        // No data available
    }

    // Data is available, read it from socket.
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error reading from socket");
        // exit(1);
    }
    else if (n1 == 0)
    {
        printf("Connection closed\n");
        return;
    } // Connection closed

    // Pritn the message from the socket.
    printf("Message from server: %s\n", buffer);
}