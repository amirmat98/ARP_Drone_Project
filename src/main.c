#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>

// Pipes
int key_pressing[2];

int main(int argc, char *argv[])
{
    // PIDs
    pid_t server_pid;
    pid_t window_pid;
    pid_t km_pid;
    pid_t drone_pid;
    pid_t wd_pid;

    create_pipes();

    // File des
    char key_manager_des[80];
    char window_des[80];

    sprintf(key_manager_des, "%d %d", key_pressing[0], key_pressing[1]);
    sprintf(window_des, "%d %d", key_pressing[0], key_pressing[1]);

    int delay = 100000; // Time delay between next spawns
    int number_process = 0; //number of processes

    /* Server  */
    char* server_args[] = {"konsole", "-e", "./build/server", NULL};
    server_pid = create_child(server_args[0], server_args);
    number_process++;
    usleep(delay*10); // little bit more time for server

    /* Keyboard manager */
    char* km_args[] = {"konsole", "-e", "./build/key_manager", key_manager_des, NULL};
    km_pid = create_child(km_args[0], km_args);
    number_process++;
    usleep(delay);

    /* Drone */
    char* drone_args[] = {"konsole", "-e", "./build/drone", NULL};
    drone_pid = create_child(drone_args[0], drone_args);
    number_process++;
    usleep(delay);

    /* Watchdog */
    char* wd_args[] = {"konsole", "-e", "./build/watchdog", NULL};
    wd_pid = create_child(wd_args[0], wd_args);
    number_process++;
    printf("Watchdog Created\n");
    

    /* Window - Interface */
    char* window_args[] = {"konsole", "-e", "./build/interface", window_des, NULL};
    window_pid = create_child(window_args[0], window_args);
    number_process++;
    usleep(delay);

    /* Wait for all children to close */
    for (int i = 0; i < number_process; i++)
    {
        int pid, status;
        pid = wait(&status);

        if (WIFEXITED(status))
        {
            printf("Child process with PID: %i terminated with exit status: %i\n", pid, WEXITSTATUS(status));
        }
    }

    printf("All child processes closed, closing main process...\n");
    return 0;

}

void create_pipes()
{
    pipe(key_pressing);

    printf("Pipes Succesfully created");
}

int create_child(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
    {
        printf("Child process %s with PID: %d  was created\n", program, child_pid);
        return child_pid;
    }
    else if (child_pid == 0)
    {   
        // This is where child goes
        execvp(program, arg_list);
    }
    else
    {
        perror("Fork failed");
    }
}
