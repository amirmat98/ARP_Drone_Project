#include "targets.h"
#include "import.h"

// Pipes working with the server
int server_targets[0];
int targets_server[1];

// Sockets
int socket_fd;

int main(int argc, char *argv[])
{
    sleep(1);

    // Set Configuration
    char program_type[MSG_LEN];
    char socket_data[MSG_LEN];
    read_args_from_file("./src/configuration.txt", program_type, socket_data);
    char host_name[MSG_LEN];
    int port_num;
    printf("Program type: %s\n", program_type);

    parse_host_port(socket_data, host_name, &port_num);
    printf("Host name: %s\n", host_name);
    printf("Port number: %d\n", port_num);


    if (strcmp(program_type, "server") == 0)
    {
        exit(0);
    }

    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   
    publish_pid_to_wd(TARGETS_SYM, getpid());


    // Seed random number generator with current time
    srand(time(NULL));

    // To compare previous values
    bool obtained_dimensions = false;
    bool targets_created = false;;

    // Variables
    int screen_size_x; 
    int screen_size_y;
    float scale_x;
    float scale_y;

    int counter = 0;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int port_number, n;
    struct sockaddr_in server_address;
    struct hostent *server;

    

    port_number = port_num; // Hardcoded port number
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("ERROR opening socket");
    }
    server = gethostbyname(host_name);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_number);
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("ERROR connecting");
    }

    int c = connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    printf("Connecting %d\n", c);


    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[] = "TI";
    write_and_wait_echo(socket_fd, init_msg, sizeof(init_msg));

    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    // According to protocol, next data from server will be the screen dimensions
    char dimension_msg[MSG_LEN];
    read_and_echo(socket_fd, dimension_msg);

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
            write_and_wait_echo(socket_fd, targets_msg, sizeof(targets_msg));
            // write_to_pipe(targets_server[1], targets_msg);
            targets_created = true;
        }


        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        /* Because the targets process does not need to run continously, 
        we may use a blocking-read for the socket */

        char socket_msg[MSG_LEN];
        // We use blocking read because targets do not not continous operation
        read_and_echo(socket_fd, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0)
        {
            printf("STOP RECEIVED FROM SERVER!\n");
            printf("This program will close in 5 seconds!\n");
            fflush(stdout);
            sleep(5);
            exit(0);
        }

        else if (strcmp(socket_msg, "GE") == 0)
        {
            // printf("GE RECEIVED FROM SERVER!\n");
            fflush(stdout);
            // printf("Regenerating targets...\n");
            targets_created = false;
        }
        // Delay
        // usleep(200000); // 200ms
    }
    clean_up();

    return 0;

}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
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

void clean_up()
{
    close(targets_server[1]);
    close(server_targets[0]);
    close(socket_fd);
}