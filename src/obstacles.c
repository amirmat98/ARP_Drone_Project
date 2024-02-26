#include "obstacles.h"
#include "import.h"


// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];


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
    publish_pid_to_wd(OBSTACLES_SYM, getpid());

    // Seed random number generator with current time
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];
    int number_obstacles = 0;
    char obstacles_msg[MSG_LEN]; // Adjust the size based on your requirements

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

    prot_number = port_num; // Port number
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Error opening socket");
        // exit(1);
    }
    server = gethostbyname(host_name);
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

    char init_msg[] = "OI" // Adjust the size based on your requirements
    write_and_wait_echo(socket_fd, init_msg, sizeof(init_msg));

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
        /* Step 1: READ THE DATA FROM SERVER */
        /////////////////////////////////////////////////////

        char socket_msg[MSG_LEN]; // Adjust the size based on your requirements
        read_and_echo_non_blocking(socket_fd, socket_msg);
        
        if (strcmp(socket_msg, "STOP") == 0)
        {
            printf("STOP RECEIVED FROM SERVER!\n");
            printf("This process will close in 5 seconds...\n");
            fflush(stdout);
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
        write_and_wait_echo(socket_fd, obstacles_msg, sizeof(obstacles_msg));

    }

    return 0;
    
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        close(obstacles_server[1]);
        close(server_obstacles[0]);
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