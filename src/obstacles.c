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

    prot_number = PORT_NUMBER; // Port number
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

    char init_msg[MSG_LEN]; // Adjust the size based on your requirements
    strcpy(init_msg, "OI");
    write_message_and_wait_for_echo(socket_fd, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, next data from server will be the screen dimensions
    float temp_scx, temp_scy;
    char dimension_msg[MSG_LEN];
    read_and_echo(socket, dimension_msg);

    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;


    while (1)
    {
        //////////////////////////////////////////////////////
        /* Step 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN]; // Adjust the size based on your requirements
        read_and_echo_non_blocking(socket_fd, socket_msg);
        
        if (strcmp(socket_msg, "STOP") == 0)
        {
            printf("STOP RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("This process will close in 5 seconds...\n");
            sleep(5);
            exit(0);
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
        write_message_and_wait_for_echo(socket_fd, obstacles_msg, sizeof(obstacles_msg));

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

    printf("[SOCKET] Received: %s\n", socket_msg);
    
    // Echo data read into socket
    bytes_written = write(socket, socket_msg, bytes_read);
    printf("[SOCKET] Echo sent: %s\n", socket_msg);

}
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

void write_message_and_wait_for_echo(int socket, char socket_msg[], size_t msg_size)
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