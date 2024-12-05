#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

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

        // Split the input command into arguments
        char *args[100]; // Array to hold the arguments
 char *token = strtok(buffer, " "); // Split by space
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " "); // Continue to split
        }
        args[i] = NULL; // Null-terminate the argument list for execvp

        // Fork a child process to execute the command
 pid_t pid = fork();
        if (pid == -1) {
            // Fork failed
            write(STDOUT_FILENO, "Fork failed\n", 11);
            continue;
        }

        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                // If execvp fails
                write(STDOUT_FILENO, "Command execution failed\n", 24);
exit(1); // Exit the child process
            }
        } else {
            // Parent process waits for the child to finish
            wait(NULL); // Wait for the child process to complete
        }
    }
return 0;
}
