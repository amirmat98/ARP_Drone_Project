#include "main.h"
#include "import.h"


// New pipes working with server (fd)
int interface_km[2];    // Serverless pipes
int interface_drone[2]; // Serverless pipes
int km_server[2];
int server_drone[2];
int interface_server[2];
int drone_server[2];
int server_interface[2];
int server_obstacles[2];
int obstacles_server[2];
int server_targets[2];
int targets_server[2];

// PIDs
pid_t server_pid;
pid_t window_pid;
pid_t km_pid;
pid_t drone_pid;
pid_t wd_pid;
pid_t logger_pid;
pid_t targets_pid;
pid_t obstacles_pid;
pid_t all_pids[9];

FILE *log_file;
char log_file_path[80];

char msg[1024];

int main(int argc, char *argv[])
{
    // Create Log File
    create_logfile();
    log_msg(log_file_path, "MAIN", "All processes created");

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    
    // Serverless pipe creation
    if (pipe(interface_km) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(interface_drone) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }

    // Pipe creation: To server
    if (pipe(km_server) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(drone_server) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(interface_server) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(obstacles_server) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(targets_server) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }

    // Pipe creation: From server
    if (pipe(server_drone) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_interface) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_obstacles) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_targets) == -1) 
    {
        log_err(log_file_path, "MAIN", "pipe");
        exit(EXIT_FAILURE);
    }


    // Passing file descriptors for pipes used on server.c
    char server_fds[160];
    sprintf(server_fds, "%d %d %d %d %d %d %d %d %d %s", km_server[0], server_drone[1], 
    interface_server[0], drone_server[0], server_interface[1], server_obstacles[1], 
    obstacles_server[0], server_targets[1], targets_server[0], log_file_path);

    // Passing file descriptors for pipes used on key_manager.c
    char key_manager_fds[160];
    sprintf(key_manager_fds, "%d %d %s", interface_km[0], km_server[1], log_file_path);

    // Passing file descriptors for pipes used on interface.c
    char interface_fds[160];
    sprintf(interface_fds, "%d %d %d %d %s", interface_km[1], server_interface[0], interface_server[1], interface_drone[1], log_file_path);

    // Passing file descriptors for pipes used on drone.c
    char drone_fds[160];
    sprintf(drone_fds, "%d %d %d %s", server_drone[0], drone_server[1], interface_drone[0], log_file_path);

    // Passing file descriptors for pipes used on obstacles.c
    char obstacles_fds[160];
    sprintf(obstacles_fds, "%d %d %s", server_obstacles[0], obstacles_server[1], log_file_path);

    // Passing file descriptors for pipes used on targets.c
    char targets_fds[160];
    sprintf(targets_fds, "%d %d %s", server_targets[0], targets_server[1], log_file_path);

    int delay = 100000; // Time delay between next spawns
    int number_process = 0; //number of processes

    /* Server  */
    char *server_args[] = {"konsole", "-e", "./build/server", server_fds, NULL};
    // char *server_args[] = {"./build/server", server_fds, NULL};
    server_pid = create_child(server_args[0], server_args);
    all_pids[number_process] = server_pid;
    number_process++;
    usleep(delay * 10); // little bit more time for server

    /* Window - Interface */
    char *window_args[] = {"konsole", "-e", "./build/map", interface_fds, NULL};
    window_pid = create_child(window_args[0], window_args);
    all_pids[number_process] = window_pid;
    number_process++;
    usleep(delay);

    /* Targets */
    // char *targets_args[] = {"konsole", "-e", "./build/targets", targets_fds, NULL};
    char *targets_args[] = {"./build/targets", targets_fds, NULL};
    targets_pid = create_child(targets_args[0], targets_args);
    all_pids[number_process] = targets_pid;
    number_process++;
    usleep(delay);

    /* Obstacles */
    // char *obstacles_args[] = {"konsole", "-e", "./build/obstacles", obstacles_fds, NULL};
    char *obstacles_args[] = {"./build/obstacles", obstacles_fds, NULL};
    obstacles_pid = create_child(obstacles_args[0], obstacles_args);
    all_pids[number_process] = obstacles_pid;
    number_process++;
    usleep(delay);

    /* Keyboard manager */
    // char *km_args[] = {"konsole", "-e", "./build/key_manager", key_manager_fds, NULL};
    char *km_args[] = {"./build/key_manager", key_manager_fds, NULL};
    km_pid = create_child(km_args[0], km_args);
    all_pids[number_process] = km_pid;
    number_process++;
    usleep(delay);

    /* Drone */
    // char *drone_args[] = {"konsole", "-e", "./build/drone", drone_fds, NULL};
    char *drone_args[] = {"./build/drone", drone_fds, NULL};
    drone_pid = create_child(drone_args[0], drone_args);
    all_pids[number_process] = drone_pid;
    number_process++;
    usleep(delay);

    /* Watchdog */
    // char *wd_args[] = {"konsole", "-e", "./build/watchdog", NULL};
    char *wd_args[] = {"./build/watchdog", log_file_path, NULL};
    wd_pid = create_child(wd_args[0], wd_args);
    all_pids[number_process] = wd_pid;
    number_process++;
    

    /* Wait for all children to close */
    for (int i = 0; i < number_process; i++)
    {
        int pid, status;
        pid = wait(&status);

        if (WIFEXITED(status))
        {
            sprintf(msg, "Child process with PID: %i terminated with exit status: %i\n", pid, WEXITSTATUS(status));
            printf("%s", msg);
            log_msg(log_file_path, "MAIN", msg);
        }
    }

    sprintf(msg, "All child processes closed, closing main process...\n");
    printf("%s", msg);
    log_msg(log_file_path, "MAIN", msg);

    clean_up();

    return 0;

}

void signal_handler(int signo, siginfo_t *siginfo, void *context)
{
    if (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT, killing all children... \n");
        printf("\n%s", msg);
        log_msg(log_file_path, "MAIN", msg);

        
        kill(server_pid, SIGKILL);
        kill(window_pid, SIGKILL);
        kill(km_pid, SIGKILL);
        kill(drone_pid, SIGKILL);
        kill(wd_pid, SIGKILL);
        kill(logger_pid, SIGKILL);
        kill(targets_pid, SIGKILL);
        kill(obstacles_pid, SIGKILL);
        

        sprintf(msg, "Closing all pipes.. \n");
        printf("%s", msg);
        log_msg(log_file_path, "MAIN", msg);

        clean_up();
        exit(1);
    }
}

int create_child(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
    {
        sprintf(msg, "Child process %s with PID: %d  was created\n", program, child_pid);
        printf("%s", msg);
        log_msg(log_file_path, "MAIN", msg);
        return child_pid;
    }
    else if (child_pid == 0)
    {   
        // This is where child goes
        execvp(program, arg_list);
        exit(0);
    }
    else
    {
        log_err(log_file_path, "MAIN", "Fork failed");
    }
}

void clean_up()
{ 
    // Interface pipes
    close(interface_km[0]);
    close(interface_drone[0]);
    close(interface_km[1]);
    close(interface_drone[1]);
    close(interface_server[0]);
    close(interface_server[1]);
    // Keyboard Manager pipes
    close(km_server[0]);
    close(km_server[1]);
    // Drone pipes
    close(drone_server[0]);
    close(drone_server[1]);
    // Server pipes
    close(server_interface[0]);
    close(server_interface[1]);
    close(server_drone[0]);
    close(server_drone[1]);
    close(server_obstacles[0]);
    close(server_obstacles[1]);
    close(server_targets[0]);
    close(server_targets[1]);
    // Targets pipes
    close(targets_server[0]);
    close(targets_server[1]);
    // Obstacles pipes
    close(obstacles_server[0]);
    close(obstacles_server[1]);
}

void create_logfile()
{
    /* Create Logfile */
    time_t current_time;
    struct tm *time_info;
    char file_name[40];  // Buffer to hold the filename

    time(&current_time);
    time_info = localtime(&current_time);

    // Format time into a string: "logfile_YYYYMMDD_HHMMSS.txt"
    strftime(file_name, sizeof(file_name), "logfile_%Y%m%d_%H%M%S.txt", time_info);

    char directory[] = "build/logs";
    sprintf(log_file_path, "%s/%s", directory, file_name);

    // Create the logfile
    log_file = fopen(log_file_path, "a");

    if (log_file == NULL)
    {
        perror("Unable to open the log file.");
        exit(1);
    }
    // close logfile
    fclose(log_file);
}