#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main() {
    // Welcome message to be displayed
    const char *welcome_message = "Welcome to ENSEA Tiny Shell.\nType 'exit' to quit.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));
 // Command buffer for storing user input data
    char buffer[BUFFER_SIZE];

    while (1) {
        // Display prompt to display continuously till 'exit' message is typed by user
        const char *prompt = "enseash % ";
        write(STDOUT_FILENO, prompt, strlen(prompt));
        
        // Read user input
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) break; // End loop if read fails

  buffer[bytes_read - 1] = '\0'; // Null-terminate string so that the string ends without a new-line command

        // Exit condition
        if (strcmp(buffer, "exit") == 0) {
            break; // break when the "exit" word is read from the user input
        }

        // Parse for input/output redirection
        char *input_file = NULL;
        char *output_file = NULL;
        char *args[BUFFER_SIZE];
        int i = 0;
 // Tokenize the input to handle arguments and redirections
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                input_file = strtok(NULL, " "); // Get file for input redirection
            } else if (strcmp(token, ">") == 0) {
                output_file = strtok(NULL, " "); // Get file for output redirection
            } else {
                args[i++] = token; // Add argument to args array
 }
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null terminate the argument list

        // Start measuring the execution time
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process

// Handle input redirection
            if (input_file) {
                int input_fd = open(input_file, O_RDONLY);
                if (input_fd == -1) {
                    perror("Input file error");
                    exit(1);
                }
                dup2(input_fd, STDIN_FILENO); // Redirect stdin to input file
                close(input_fd);
            }

            // Handle output redirection
            if (output_file) {
int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd == -1) {
                    perror("Output file error");
                    exit(1);
                }
                dup2(output_fd, STDOUT_FILENO); // Redirect stdout to output file
                close(output_fd);
            }

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror("Exec error");
                exit(1);
            }
  } else {
            // Parent process waits for child
            int status;
            waitpid(pid, &status, 0);

            // End measuring the execution time
            clock_gettime(CLOCK_MONOTONIC, &end);
            long elapsed_time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

            // Print the exit code and execution time
            if (WIFEXITED(status)) {
 int exit_code = WEXITSTATUS(status);
                char status_message[BUFFER_SIZE];
                snprintf(status_message, sizeof(status_message), "[exit:%d|%ldms]\n", exit_code, elapsed_time);
                write(STDOUT_FILENO, status_message, strlen(status_message));
            } else if (WIFSIGNALED(status)) {
                int signal_number = WTERMSIG(status);
                char status_message[BUFFER_SIZE];
                snprintf(status_message, sizeof(status_message), "[sign:%d|%ldms]\n", signal_number, elapsed_time);
                write(STDOUT_FILENO, status_message, strlen(status_message));
            }
        }
    }
return 0;
}
