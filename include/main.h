#ifndef MAIN_H    
#define MAIN_H


int create_child(const char *program, char **arg_list);


/**
 * Closes all pipes created during the program's execution
*/
void close_all_pipes();



#endif // MAIN_H