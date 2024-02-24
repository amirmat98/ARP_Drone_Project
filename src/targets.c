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

    port_number = 2000; // Hardcoded port number
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0)
    {
        error("ERROR opening socket");
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
        error("ERROR connecting");
    }

    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char msg[MSG_LEN];
    strcpy(msg, "TI");
    write_message_and_wait_for_echo(socket, msg);


    while(1)
    {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_server;
        FD_ZERO(&read_server);
        FD_SET(server_targets[0], &read_server);
        
        char server_msg[MSG_LEN] = {0};

        int ready = select(server_targets[0] + 1, &read_server, NULL, NULL, &timeout);
        if (ready == -1) 
        {
            perror("Error in select");
            break; // Exit loop on select error
        }

        if (ready > 0 && FD_ISSET(server_targets[0], &read_server)) 
        {
            ssize_t bytes_read = read(server_targets[0], server_msg, MSG_LEN);
            if (bytes_read > 0) 
            {
                server_msg[bytes_read] = '\0'; // Ensure string is null terminated
                float temp_scx, temp_scy;
                sscanf(server_msg, "I2:%f,%f", &temp_scx, &temp_scy);
                screen_size_x = (int)temp_scx;
                screen_size_y = (int)temp_scy;
                printf("Obtained from server: %s\n", server_msg);
                fflush(stdout);
                obtained_dimensions = true;
            }
        }
        if(obtained_dimensions == false)
        {
            continue;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: TARGET GENERATION & SEND DATA*/
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
            write_to_pipe(targets_server[1], targets_msg);
            targets_created = true;
        }

        // Delay
        usleep(500000);  

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

char* read_and_echo(int socket)
{
    int n1, n2;
    static char buffer[MSG_LEN];
    bzero(buffer, MSG_LEN);

    // Read the message from the socket
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error in reading from socket");
    }
    printf("Message obtained: %s\n", buffer);

    // Echo data read into socket
    n2 = write(socket, buffer, n1);
    return buffer;
}
char* read_and_echo_non_blocking(int socket)
{
    static char buffer[MSG_LEN];
    fd_set read_set;
    struck timeval timeout;
    int n1, n2, ready;

    // clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to be read
    FD_ZERO(&read_set);
    FD_SET(socket, &read_set);

    // Set the timeout for select
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Use select to wait for data to be available
    ready = select(socket + 1, &read_set, NULL, NULL, &timeout);
    if (ready < 0)
    {
        perror("Error in select");
    }
    else if (ready == 0)
    {
        return NULL;
    }

    // Data is available, read the message
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error in reading from socket");
    }
    else if (n1 == 0)
    {
        return NULL;
        // Connectiiion closed
    }

    // Print the received message
    printf("Message obtained: %s\n", buffer);

    // Echo the message back to the client
    n2 = write(socket, buffer, n1);
    if (n1 < 0)
    {
        perror("Error in writing to socket");
    }

    return buffer;
}
void write_message_and_wait_for_echo(int socket, char *message)
{
    static char buffer[MSG_LEN];
    fd_set read_set;
    struck timeval timeout;
    int n1, n2, ready;

    n2 = write(socket, message, sizeof(msg));
    if (n1 < 0)
    {
        perror("Error in writing to socket");
    }
    printf("Data sent to server: %s\n", message);

    // clear the buffer
    bzero(buffer, MSG_LEN);

    // Initialize the set of file descriptors to be read
    FD_ZERO(&read_set);
    FD_SET(socket, &read_set);

    // set the timeout for select
    timeout.tv_sec = 0;
    timeout.tv_usec =  300000;

    // Use select to wait for data to be available
    ready = select(socket + 1, &read_set, NULL, NULL, &timeout);
    if (ready < 0)
    {
        perror("Error in select");
    }
    else if (ready == 0)
    {
        perror("Timeout from server. Connection lost");
        return;
    }

    // Data is available, read the message
    n1 = read(socket, buffer, MSG_LEN - 1);
    if (n1 < 0)
    {
        perror("Error in reading from socket");
    }
    else if (n1 == 0)
    {
        perror("Connection closed!");
        return;
    } // Connection closed

    // Print the received message
    printf("Response from server: %s\n", buffer);
}