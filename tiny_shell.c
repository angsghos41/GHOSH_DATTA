#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define BUFFER_SIZE 1024

int main() {
    // Welcome message to be displayed
 const char *welcome_message = "Welcome to ENSEA Tiny Shell.\nType 'exit' to quit.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message)); // writes data directly to file descriptor
    
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

        // Start measuring execution time
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time); // Get start time

        pid_t pid = fork(); // Fork a new process

 if (pid == 0) { // Child process
            // Execute the command
            execlp(buffer, buffer, (char *)NULL); // Execute the command entered by the user
            // If execlp fails, exit with an error code
            exit(1);
        } else if (pid > 0) { // Parent process
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to complete
 // Measure end time
            clock_gettime(CLOCK_MONOTONIC, &end_time);

            // Calculate elapsed time in milliseconds
            long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000L + (end_time.tv_nsec - start_time.tv_nsec) / 1000000L;

            // Prepare status message
            char status_message[BUFFER_SIZE];

            if (WIFEXITED(status)) { // If the command exited normally
                int exit_code = WEXITSTATUS(status);
                snprintf(status_message, sizeof(status_message), "[exit:%d|%ldms]", exit_code, elapsed_time);
            } else if (WIFSIGNALED(status)) { // If the command was terminated by a signal
                int signal_number = WTERMSIG(status);
                snprintf(status_message, sizeof(status_message), "[sign:%d|%ldms]", signal_number, elapsed_time);
            }

            // Display the status message after the command execution
write(STDOUT_FILENO, status_message, strlen(status_message));
            write(STDOUT_FILENO, "\n", 1);
        } else {
            // If fork fails
            write(STDOUT_FILENO, "Fork failed.\n", 13);
        }
    }
return 0;
}

