# Description
[Università degli studi di Genova](https://unige.it/en/ "University of Genova")

[Advanced and Robotic Programming](https://corsi.unige.it/en/off.f/2023/ins/66535)

Professor: [Renato Zaccaria](renato.zaccaria@unige.it "Renato Zaccaria")

Student: [AmirMahdi Matin](https://github.com/amirmat98 "AmirMahdi Matin")  - 5884715 - Robotics Engineering

# ARP - Assignment 3 (Not Compleate)

# Table of Content
- [Project Summary](#Project-Summary)
- [How to use](#How-to-use)
    - [Installation & Usage](#Installation-&-Usage)
    - [Operational Instructions and Controls](Operational-Instructions-and-Controls)
- [Overview](#Overview)
    - [Main](#Main)
    - [Server](#Server)
    - [Watchdog](#Watchdog)
    - [Interfaces](#Interfaces)
    - [Keyboard manager](#Keyboard-manager)
    - [Drone](#Drone)
    - [Targets](#Targets)
    - [Obstacles](#Obstacles)
    - [Logger](#Logger)


## Project Summary
This is the Third component of the University of Genova's Advanced and Robot Programming course undertaking. The individual endeavors of AmirMahdi Matin have contributed to this endeavour.

## How to use

### Essential requirements
It has been guaranteed that the program is compatible with the Ubuntu operating system. Additionally, the following external libraries are required for the compilation of code: konsole and ncurses.

```
sudo apt-get install libncurses5-dev libncursesw5-dev
```
```
sudo apt-get install konsole
```

### Installation & Usage
For the purpose of configuration, the project makes use of a `Makefile`. It is possible to execute the command `make` within the project directory in order to construct the executables. Use the command `make run` to start the game, which will cause numerous konsole processes to be launched simultaneously. Running the command `make clean` will allow you to uninstall the executables.

The application can be run in three primary modes:
`Autonomous` refers to the fact that the game can be played on a local level.
`Server` is required to accept connections with two external clients (obstacles and targets) in order for the game to be played. However, the server is capable of playing the game.
`Client` The only procedures that will be operational are those that involve obstacles and targets. The anticipated data will be stored on a distant server that they will supply. No game can be played on the machine that is located locally.

In order to modify the execution mode, it is essential to access the `config.txt` file and carry out the necessary modifications to the sole line that is not commented.
In the same file, the instructions are located at the very top of the document.


### Operational Instructions and Controls
The drone in the game can be controlled using the following keybindings:

- `q`, `w`, `e`: Represent upward movement in different directions.
- `a`, `s`, `d`: Represent left, stop, and right movements, respectively.
- `z`, `x`, `c`: Represent downward movement in different directions.

To halt all forces on the drone, the `s` key, which is located in the center of the keyboard, is utilized. Despite this, the drone will continue to move for a little period of time before coming to a full halt. This is because of the force of inertia. In addition, you have the ability to control the drone by pressing the `h` key, which allows you to simulate an emergency abort in real life. The application can be stopped by pressing the special key `p` if it is being utilized.

## Overview
In the middle of the Konsole window that is now running the interface process, a plus sign (+) that represents a drone will appear. 
In addition, we will observe barriers represented by asterisks (*) and targets represented by numbers (1, 2, 3). Both of these will be displayed on the screen.

The objective of the game is to ensure that all of the targets are reached in the correct order while simultaneously attempting to avoid the obstacles that appear at random over the entirety of the game.

An update to the calculation of the score will be made in real time. It is possible to obtain the maximum number of points if a task is accomplished within a predetermined amount of time. If it takes the player an excessive amount of time to accomplish a goal, they will receive fewer points. On top of that, points will be removed from a player's total if they run into an obstacle.

After all of the objectives have been accomplished, the game will restart, and the player will be given a fresh set of objectives to complete.

[//]: <> ("Add Picture")

A sketch is used to show the organizational structure of the project. A number of components make up the architecture, which includes the Main, the Server, the Window (which represents the user interface), the Keyboard Manager, the Drone, the Targets, the Obstacles, and the Watchdog. Within the context of the project's overall functioning, each component fulfills a distinct function.

additional information regarding each of the components that were stated earlier. kindly refer to the description that is provided below.


### Main
The primary process is the progenitor of further processes. Through the utilization of the `fork()` function, it generates child processes and executes them under a wrapper program known as `konsole` to display the current status and debugging messages until an additional thread or process is created for the purpose of collecting log messages.

In order to create every pipe that may be utilized within the program, the `pipe()` function is utilized. Additionally, the file descriptors are supplied to the arguments of the processes that are formed. The read or write end of each pipe is only assigned to a single process at any one time.

Immediately following the creation of offspring, the process remains in an indefinite while loop, waiting for the termination of all other processes. When this occurs, the process terminates upon itself.


### Server
Server procedure is the most important aspect of this project. When it reads the data, it is able to transmit it to the appropriate receivers since it maintains all communications involving pipelines among all processes. This allows it to send the data to the appropriate recipients. The execution of this is carried out within a controlled endless loop, the termination of which is contingent upon the watchdog control of the program. 

Furthermore, the server is responsible for managing incoming connections that originate from either the network or the same computer. It employs the socket function to communicate particularly with the processes that are causing obstacles and targets.

It is possible for the server to examine the contents of each message that it has received in order to determine the appropriate destination for the message when it happens to be essential. Because of this, every one of the processes will only read the data that is specifically designed for them. 



### Watchdog
It is the responsibility of the Watchdog to keep an eye on the "health" of all of the processes, which basically means determining whether or not the processes are active and not closed.

The process identifiers (PIDs) of processes are obtained from a specific shared memory segment during the initialization phase. It is important to keep in mind that the wrapper process Konsole is being executed in the main process, which means that it is not feasible to obtain the true PID from the `fork()` function in the main process, at least at this time.

Once the process enters the while loop, it sends a signal known as "SIGUSR1" to all of the processes in order to determine whether or not they are still alive. The watchdog receives a response with `SIGUSR2`, and they are aware of its PID because of the `siginfo_t` function. Because of this, the counter for programs to respond is reset to zero. In the event that the counter for any of them hits the threshold, Watchdog will send a signal known as "SIGINT" to all of the processes and then quit, ensuring that all of the semaphores it was utilizing are closed. It is the same thing that takes place whenever Watchdog is disrupted.


### Interface
By establishing a graphical environment, the interface process is responsible for managing user interaction within the `Konsole` application software. This environment is intended to collect inputs from users, which are physically represented by key presses, and then display the most recent state of the application after receiving those inputs.

In order to get this process started, the relevant signals are initialized, and pipe file descriptors are obtained. This lays the framework for the execution of the process. The server will be the sole source of data that the interface will get, and the drone, targets, and obstacles processes will be the origin of this data.

For the purpose of constructing the graphical user interface, the ncurses library is utilized. In the beginning, the program utilizes the `initscr()` function. Subsequently, the terminal dimensions are obtained by utilizing the `getmaxyx()` function. Finally, a box is drawn around the interface by utilizing the `box()` function. The initial position of the drone is established somewhere in the middle of this box, which serves as a representation of the boundaries of the functional region within which the drone is allowed to roam.

Subsequently, the application becomes trapped in an indefinite loop, continuously monitoring the key presses of the user with the assistance of the `getch()` function. Whenever a movement action occurs, it simultaneously updates the position of the drone on the screen by using the `mvaddch()` function. Consequently, this guarantees that the user can engage with the drone in real time and visualize its motions.

This process also handles logic surrounding the present state of the game, for which it continuously analyzes the position of the drone, targets, and obstacles in order to generate a score that will be updated on the screen for the player to see. This score will be displayed for the player whenever the game is updated.


### Keyboard manager
It is the keyboard manager process that acts as the connection between the user interface and the computation of the movements of the drone. The key that was pressed by the user is retrieved from the shared memory, and then it is translated into integer values that indicate the force that was applied in each possible direction. These values are then conveyed to the procedure that is being carried out by the drone.

Within an endless while loop, the iteration duration is controlled by a `do_while`, in which the function `select()` is utilized inside of it, to wait for the readiness of the pipe to allow execution of the rest of the code. Several `if` statements are contained within a function that is responsible for determining the precise action that is to be taken. Based on these conditions, values are allocated to variables x or y, which represent movement along the horizontal or vertical axis within the window. These variables are used to represent movement. There are three possible values that are sent, which are `[-1, 0, 1]`. These values represent the quantity and direction of movement. Any other value that falls outside of this range would indicate a particular action that is not related to movement that is comprehended by the drone process.


### Drone
Within the interface working area (window), the drone process is the one responsible for the physics of the drone, as well as its movement, bounds, and conditional events. It makes certain that the drone acts and travels appropriately on the window that is drawn by the interface. For this purpose, physical equations have been developed in order to mimic movement within a subset of specific parameters that have been pre-defined. Simultaneously, the drone engages in interactions with exterior objects, such as targets and barriers, which result in the application of extra pressures that result in a more dynamic control for the drone.

Several variables, including position, velocity, force, and acceleration, are defined in this context. Only the initial position values at which it drew the drone on the screen are required to be provided by the interface. From that point forward, this mechanism alone will be responsible for determining all of the prospective positions that the drone will take.

With the exception of the procedures that are required for reading or writing data to the server, the main loop does not make use of any special functions. The `diffrentioal equation` is the default choice at the beginning of the program, and it contains the drone dynamic formulas that cause the drone to move with inertia and viscous resistance. This method is used to move the drone through the screen. A formula was computed using Kathib's model in order to derive both the repulsion and attraction forces that were operating on the drone. This was done in order to account for the external forces.


### Targets
In order to communicate with the server, the targets process makes use of sockets. To accomplish this, it is necessary to retrieve the screen dimensions of the window from the server. After they have been received, the logic that is responsible for producing the target is carried out once and then immediately transmitted. An additional set of target coordinates will be sent to the server in the event that a message containing the letter "GE" is retrieved.

There is a blend of randomness and order in the calculation that determines where the targets are located on the screen. For any given region, there are six sections that are formed. Despite the fact that the coordinates of the targets are generated using the `rand()` function, they are nonetheless arranged in such a way that they are dispersed evenly across those sections. This ensures that the player will never obtain complete areas that are targetless, allowing them to make effective use of the entire window.


### Obstacles
IPC with the server is accomplished through the use of sockets by the obstacles process. In addition to this, it is necessary to determine the screen dimensions of the window before running the main loop. On the other hand, there are a number of significant differences between the targets and these in terms of how they are generated on the battlefield. 

In the beginning, they are formed in a completely random manner across the entirety of the screen. Secondly, because the obstacles emerge and disappear at random intervals, it never stops sending data to the server, which updates the new coordinates for each obstacle. This behavior is a result of the random intervals. The behaviour of the production of targets is directly influenced by a large number of pre-defined variables, the majority of which are associated with the minimum and maximum amount of time that they will appear on the screen, as well as the rate at which they are formed.


### Logger
For the purpose of logging, we have devised a method in which, rather than utilizing a specialized procedure that would record everything, we generate a single log session file in the format of text. This file is named with the exact timestamp that corresponds to the beginning of the session, such as "logfile_20240226_200817.txt." The main process is responsible for creating the logfile, and the remaining processes are able to add their messages to it by opening the file and attaching their log message to it.
Logging can be done in two different methods. The `log_msg()` function allows processes to log a message of type info `[INFO]`, while the `log_err()` function is used to log an error.

