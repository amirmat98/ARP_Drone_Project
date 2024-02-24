#include "targets.h"
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
int server_targets[0];
int targets_server[1];

int main(int argc, char *argv[])
{
    sleep(1);

    get_args(argc, argv);

    srand(time(NULL));

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // To compare previous values
    bool obtained_dimensions = false;
    bool targets_created = false;;

    // Variables
    int screen_size_x; int screen_size_y;
    float scale_x;
    float scale_y;

    int counter = 0;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int socket, port_number, n;
    struct sockaddr_in server_address;
    struct hostent *server;

    port_number = PORT_NUMBER; // Hardcoded port number
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0)
    {
        perror("ERROR opening socket");
    }
    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no host named 'localhost'\n");
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_number);
    if (connect(socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("ERROR connecting");
    }

    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[MSG_LEN];
    strcpy(init_msg, "TI");
    write_message_and_wait_for_echo(socket, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, next data from server will be the screen dimensions
    char dimension_msg[MSG_LEN];
    read_and_echo(socket, dimension_msg);
    float temp_scx, temp_scy;
    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;

    while(1)
    {

        //////////////////////////////////////////////////////
        /* SECTION 1: TARGET GENERATION & SEND DATA*/
        /////////////////////////////////////////////////////
        
        // Array to store generated targets
        Target targets[MAX_TARGETS];
        // Set seed for random number generation
        srand(time(NULL));

        if(targets_created == false)
        {
            // Generate random order for distributing targets across sectors
            int order[MAX_TARGETS];
            generate_random_order(order, MAX_TARGETS);

            // Generate targets within each sector
            for (int i = 0; i < MAX_TARGETS; ++i) 
            {
                // Generate the target
                Target temp_target = generate_target(order[i], screen_size_x, screen_size_y);
                // Store the target
                targets[i] = temp_target;
            }

            // make target messages
            char targets_msg[MSG_LEN];
            make_target_msg(targets, targets_msg);

            // Send the data to the server
            write_message_and_wait_for_echo(socket, targets_msg, sizeof(targets_msg));
            // write_to_pipe(targets_server[1], targets_msg);
            targets_created = true;
        }


        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN];
        // We use blocking read because targets do not not continous operation
        read_and_echo_non_blocking(socket, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0)
        {
            printf("STOP RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("This program will close in 5 seconds!\n");
            sleep(5);
            exit(0);
        }

        else if (strcmp(socket_msg, "GE") == 0)
        {
            printf("GE RECEIVED FROM SERVER!\n");
            fflush(stdout);
            printf("Regenerating targets...\n");
        }
        // Delay
        usleep(200000); // 200ms
    }
    return 0;

}

void generate_random_cordinates(int sector_width, int sector_height, int *x, int *y)
{
    *x = (int)(0.05 * sector_width) + (rand() % (int)(0.95 * sector_width));
    *y = (int)(0.05 * sector_height) + (rand() % (int)(0.95 * sector_height));
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_targets[0], &targets_server[1]);
}

void generate_random_order(int *order, int max_targets)
{
    for (int i = 0; i < max_targets; ++i) 
    {
        order[i] = i;
    }
    for (int i = max_targets - 1; i > 0; --i) 
    {
        int j = rand() % (i + 1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }
}

Target generate_target(int order, int screen_size_x, int screen_size_y)
{
    Target target;

    // Sector dimensions
    const int sector_width = screen_size_x / 3;
    const int sector_height = screen_size_y / 2;

    // Determine sector based on the random order
    int sector_x = (order % 3) * sector_width;
    int sector_y = (order / 3) * sector_height;

    // Generate random coordinates within the sector
    generate_random_cordinates(sector_width, sector_height , &target.x, &target.y);

    // Adjust coordinates based on the sector position
    target.x += sector_x;
    target.y += sector_y;

    // Ensure coordinates do not exceed the screen size
    target.x = target.x % screen_size_x;
    target.y = target.y % screen_size_y;

    return target;

}

void make_target_msg(Target *targets, char *message)
{
    // Construct the targets_msg string
    int offset = sprintf(message, "T[%d]", MAX_TARGETS);
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        offset += sprintf(message + offset, "%.3f,%.3f", (float)targets[i].x, (float)targets[i].y);

        // Add a separator unless it's the last element
        if (i < MAX_TARGETS - 1)
        {
            offset += sprintf(message + offset, "|");
        }
    }
    message[offset] = '\0'; // Null-terminate the string
    printf("%s\n", message);
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