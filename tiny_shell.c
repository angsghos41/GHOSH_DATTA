
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main() {
    // Print the welcome message once
    const char *welcome_message = "Welcome to ENSEA Tiny Shell.\nType 'exit' to quit.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));

    char buffer[BUFFER_SIZE];

    while (1) {
        // Clear the screen before displaying the prompt
        system("clear");
        // Display the prompt
        const char *prompt = "enseash % ";
        write(STDOUT_FILENO, prompt, strlen(prompt));

        // Read user input (Ctrl+D triggers EOF, which returns -1)
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);

        // If EOF (Ctrl+D) is pressed, exit the shell
        if (bytes_read == 0) {
 // Print the goodbye message for Ctrl+D
            const char *bye_message = "Bye bye...\n";
            write(STDOUT_FILENO, bye_message, strlen(bye_message));
            break;  // Exit the loop
        }

        if (bytes_read <= 0) break;  // Exit if reading fails

        buffer[bytes_read - 1] = '\0';  // Null-terminate the string

// Exit condition when 'exit' is typed
        if (strcmp(buffer, "exit") == 0) {
            const char *bye_message = "Bye bye...\n";
            write(STDOUT_FILENO, bye_message, strlen(bye_message));
            break;  // Exit the loop
        }

        // Tokenize the input string
        char *args[100];
        char *token = strtok(buffer, " ");  // Split input by spaces

int i = 0;
        while (token != NULL) {
            args[i++] = token;  // Store tokens (command and arguments)
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Null-terminate the argument list

        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);  // Execute the command
            perror("execvp");  // If execvp fails
            exit(EXIT_FAILURE);  // Exit child process on failure
        } 
 else if (pid > 0) {
            wait(NULL);  // Parent waits for child process to finish
        } 
        else {
            perror("fork");  // Print error if fork fails
        }
    }

    return 0;
}
