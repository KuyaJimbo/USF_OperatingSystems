// Name: James Ocampo
// UID: U69643093
// NetID: jamesocampo
// Description: this program mimics a shell by executing commands (including built-in commands) and handling parallel commands, redirection, and paths. 
// It also handles program errors and frees allocated memory when the shell is terminated.

#define _GNU_SOURCE     // Required for getline
#include <fcntl.h>      // Required for open, O_WRONLY, O_CREAT, O_TRUNC, 0644
#include <stdio.h>      // Required for printf, snprintf, fprintf, stderr, stdout
#include <stdlib.h>     // Required for malloc, free, exit
#include <string.h>     // Required for strtok, strtok_r, strcmp, strlen, strdup, strtok, strtok_r
#include <sys/wait.h>   // Required for waitpid
#include <unistd.h>     // Required for access, execv, fork, getpid, pipe, dup2, close, chdir
#include <ctype.h>      // Required for isspace

// define constants
#define MAX_INPUT_SIZE 255
#define MAX_ARGS 64
#define INITIAL_PATH_SIZE 1

// Handle Error Messages (Item 6 - Program Errors)
char error_message[30] = "An error has occurred\n";
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Handle Paths (Item 2 - Paths)
char **path;                        // path is an pointer to an array of strings
int path_size;                      // store the size of the path
void initialize_path() {
    path = malloc(sizeof(char *));  // allocate memory for the path
    path[0] = strdup("/bin");       // set the initial path to "/bin"
    path_size = INITIAL_PATH_SIZE;  // set the initial path size to 1
}

void update_path(char **new_path, int new_size) {
    for (int i = 0; i < path_size; i++) {           // free each path element
        free(path[i]);
    }
    free(path);                     // free the path array
    path = new_path;                // set the new path
    path_size = new_size;           // set the new path size
}

// Helper function to parse and validate a single command
int parse_command(char *input, char **args, char **output_file) {
    int arg_count = 0;                                      // start counting arguments at 0
    *output_file = NULL;                                    // keep track of an output file if found

    char *token = strtok(input, " \t");                     // tokenize the input by splitting at the space or tab character
    int redirection_found = 0;                              // flag to check if redirection is found

    while (token != NULL && arg_count < MAX_ARGS - 1) {     // store each token in the arguments array unless the max number of args is reached
        if (strcmp(token, ">") == 0) {                      // check if the token is a redirection
            if (redirection_found || arg_count == 0) {      // if there is a ">" despite already finding redirection or if there are no arguments before the found redirection
                return -1;                                  // Error: More than one redirection or no arguments before redirection
            }
            redirection_found = 1;                          // set the redirection flag
            token = strtok(NULL, " \t");                    // get the next token
            if (token == NULL) {                            // if no token is found after the redirection
                return -1;                                  // Error: No filename after redirection
            }
            *output_file = token;                           // store the output file
        } else if (redirection_found) {                     // check if redirection is found
            return -1;                                      // Error: More arguments after redirection
        } else {
            args[arg_count++] = token;                      // store the token in the arguments array
        }
        token = strtok(NULL, " \t");                        // get the next token
    }
    args[arg_count] = NULL;                                 // set the last argument to NULL
    return arg_count;                                       
}

int execute_parallel_commands(char **commands, int command_count) {
    pid_t pids[command_count];                              // store the process IDs of the child processes

    for (int i = 0; i < command_count; i++) {               // for each command
        char *args[MAX_ARGS];                               // create a new array to store the arguments
        int arg_count = 0;                                  // start counting arguments at 0
        char *output_file = NULL;                           // keep track of an output file if found
        char *token = strtok(commands[i], " \t");           // tokenize the command by splitting at the space or tab character

        while (token != NULL && arg_count < MAX_ARGS - 1) { // store each token in the arguments array unless the max number of args is reached
            if (strcmp(token, ">") == 0) {                  // check if the token is a redirection
                token = strtok(NULL, " \t");                // get the next token
                if (token != NULL && output_file == NULL) { // if there is a token after the redirection and no output file is found
                    output_file = token;                    // store the output file
                } else {                                    // if there is no token after the redirection or an output file is already found
                    print_error();                          // print an error message
                    return -1;                              // return an error
                }
            } else {
                args[arg_count++] = token;                  // store the token in the arguments array
            }
            token = strtok(NULL, " \t");                    // get the next token
        }
        args[arg_count] = NULL;                             // set the last argument to NULL

        if (arg_count == 0) continue;                       // if there are no arguments, continue to the next command

        pid_t pid = fork();                                 // create a new process for each command
        // Child Process
        if (pid == 0) {
            execute_command(args[0], args, output_file);    // execute the command
            exit(0);
        } 
        // Parent Process
        else if (pid > 0) {
            pids[i] = pid;                                  // store the process ID
        } 
        // Fork Failed
        else {
            print_error();
            return -1;
        }
    }

    // each command has been forked, now wait for all child processes to complete
    for (int i = 0; i < command_count; i++) {
        int status;
        waitpid(pids[i], &status, 0); // wait for the child process to complete
    }

    return 0;
}

// Execute a single command 
int execute_command(char *command, char **args, char *output_file) {
    for (int i = 0; i < path_size; i++) {                                           // for each path in the path array
        char full_path[MAX_INPUT_SIZE];                                             // create a buffer to store the full path
        snprintf(full_path, sizeof(full_path), "%s/%s", path[i], command);          // concatenate the path and command to create the full path

        if (access(full_path, X_OK) == 0) {                                         // check if the full path is executable
            pid_t pid = fork();                                                     // create a new process
            // Child Process
            if (pid == 0) {
                if (output_file != NULL) {                                          // if an output file is found
                    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // open the output file or create it if it doesn't exist (truncate/empty it if it does)
                    if (fd == -1) {                                                 // file cannot be opened
                        print_error();
                        exit(1);
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {                        // file descriptor cannot be duplicated
                        print_error();
                        exit(1);
                    }
                    close(fd);              // close the file descriptor
                }
                execv(full_path, args);     // execute the command (returns only if an error occurs)
                print_error();              // print an error message if the command cannot be executed
                exit(1);
            } 
            // Parent Process
            else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                return 0;
            } 
            // Fork Failed
            else {
                print_error();
                return -1;
            }
        }
    }
    print_error();
    return -1;
}

// Helper function to trim leading and trailing whitespace
char* trim(char* str) {
    if (!str) return NULL;                                  // Handle NULL pointer    
    while (isspace((unsigned char)*str)) str++;             // Trim leading whitespace
    if (*str == 0) return str;                              // check if the string is empty
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    return str;
}

// Helper function to check if a string is empty or contains only whitespace
int is_empty_or_whitespace(const char* str) {
    while (*str) {                              // iterate through the string
        if (!isspace((unsigned char)*str))      // check if the character is not a whitespace
            return 0;                           // return false
        str++;                                  // move to the next character
    }
    return 1;                                   // return true if the string is empty or contains only whitespace
}

int main(int argc, char *argv[]) {
    (void)argv;                         // suppress unused variable warning

    if (argc > 1) {                     // check if there are any arguments
        print_error();
        exit(1);
    }
    initialize_path();                  // initialize the path

    char *input = NULL;                 // store the input
    size_t input_size = 0;              // store the size of the input
    ssize_t line_length;                // store the length of the input

    while (1) {
        printf("rush> ");               // print the prompt
        fflush(stdout);                 // flush the output buffer to ensure the prompt is displayed

        line_length = getline(&input, &input_size, stdin);  // read the input from the user

        if (line_length == -1) {        // check if the input is empty
            break;
        }

        if (input[line_length - 1] == '\n') {   // remove the newline character from the input
            input[line_length - 1] = '\0';      // replace the newline character with a null terminator
        }

        char *trimmed_input = trim(input);              // trim leading and trailing whitespace from the input
        if (is_empty_or_whitespace(trimmed_input)) {    // check if the input is empty or contains only whitespace
            continue;
        }

        // Handle Parallel Commands if found (Item 5 - Parallel Commands)
        char* parallel_commands[MAX_ARGS];                          // store the parallel commands
        int parallel_count = 0;                                     // store the number of parallel commands
        char* saveptr;                                              // keep track of the current position in the string for strtok_r
        char* token = strtok_r(trimmed_input, "&", &saveptr);       // tokenize the input by splitting at the "&" character

        while (token != NULL && parallel_count < MAX_ARGS) {        // store each token in the parallel commands array unless the max number of parallel commands is reached
            char* trimmed_token = trim(token);                      // trim leading and trailing whitespace from the token
            if (!is_empty_or_whitespace(trimmed_token)) {           // check if the token is not empty or contains only whitespace
                parallel_commands[parallel_count++] = trimmed_token;// store the token in the parallel commands array
            }
            token = strtok_r(NULL, "&", &saveptr);                  // get the next token
        }

        if (parallel_count == 0) {
            continue;  // All parallel commands were empty
        }

        if (parallel_count > 1) {
            execute_parallel_commands(parallel_commands, parallel_count);   // execute the parallel commands
            continue;
        }

        // Handle Single Command
        char *args[MAX_ARGS];                                                     // store the arguments
        char *output_file = NULL;                                                 // store the output file if found
        int arg_count = parse_command(parallel_commands[0], args, &output_file);  // parse the command

        if (arg_count == -1) {      // check if there was an error parsing the command
            print_error();
            continue;
        }

        if (arg_count == 0) {       // check if there are no arguments
            continue;
        }

        // Handle Built-in Commands (Item 3 - Built-in Commands)
        if (strcmp(args[0], "exit") == 0) {         // check if the command is "exit"
            if (arg_count > 1) {                    // should be the only argument
                print_error();
            } else {
                break;
            }
        } else if (strcmp(args[0], "cd") == 0) {    // check if the command is "cd"
            if (arg_count != 2) {                   // should have exactly 1 argument
                print_error();
            } else if (chdir(args[1]) != 0) {       // if change in directory is unsuccessful
                print_error();
            }
        } else if (strcmp(args[0], "path") == 0) {                          // check if the command is "path"
            char **new_path = malloc(sizeof(char *) * (arg_count - 1));     // allocate memory for the new path
            for (int i = 1; i < arg_count; i++) {                           // for each path in the arguments
                new_path[i - 1] = strdup(args[i]);                          // store the path in the new path
            }
            update_path(new_path, arg_count - 1);                           // update the path
        } else {
            execute_command(args[0], args, output_file);                    // execute the command
        }
    }

    // After the shell is terminated, free the allocated memory
    free(input);    // free the input buffer

    for (int i = 0; i < path_size; i++) {   // free each path element
        free(path[i]);
    }
    free(path);    // free the path array

    return 0;
}
