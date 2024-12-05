#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void write_number_to_fd(int fd, int number) {
    // Convert the number to a string and write it to the given file descriptor
    char num_buffer[20];
    int len = 0;

    // Handle 0 explicitly
    if (number == 0) {
        num_buffer[len++] = '0';

   } else {
        // Handle negative numbers (not required here, but added for completeness)
        if (number < 0) {
            num_buffer[len++] = '-';
            number = -number;
        }

        // Convert the number to a string (reverse order)
        char temp[20];
        int temp_len = 0;
        while (number > 0) {
            temp[temp_len++] = '0' + (number % 10);
            number /= 10;
        }

 // Reverse the string into num_buffer
        for (int i = temp_len - 1; i >= 0; i--) {
            num_buffer[len++] = temp[i];
        }
    }

    write(fd, num_buffer, len);
}

int main() {
    // Welcome message
    const char *welcome_message = "Welcome to ENSEA Tiny Shell.\nType 'exit' to quit.\n";
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));

 char buffer[BUFFER_SIZE];
    int last_status = 0; // To store the return code or signal of the last command

    while (1) {
        // Construct and display the prompt
        if (WIFEXITED(last_status)) {
            write(STDOUT_FILENO, "enseash [exit:", 14);
            write_number_to_fd(STDOUT_FILENO, WEXITSTATUS(last_status));
            write(STDOUT_FILENO, "] % ", 4);
        } else if (WIFSIGNALED(last_status)) {
 write(STDOUT_FILENO, "enseash [sign:", 14);
            write_number_to_fd(STDOUT_FILENO, WTERMSIG(last_status));
            write(STDOUT_FILENO, "] % ", 4);
        } else {
            write(STDOUT_FILENO, "enseash % ", 10);
        }

        // Read user input
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) break; // Exit the loop if reading fails
  buffer[bytes_read - 1] = '\0'; // Null-terminate the input string

        // Exit condition
        if (strcmp(buffer, "exit") == 0) {
            break; // Exit the shell if 'exit' is typed
        }

        // Fork a child process to execute the command
        pid_t pid = fork();

        if (pid == 0) { // Child process
            // Execute the command
            execlp(buffer, buffer, (char *)NULL);

   // If execlp fails, write an error message and exit
            const char *error_message = "Command not found\n";
            write(STDOUT_FILENO, error_message, strlen(error_message));
            exit(1);
        } else if (pid > 0) { // Parent process
            // Wait for the child process and retrieve its status
            wait(&last_status);
        } else {
const char *fork_error = "Fork failed\n";
            write(STDOUT_FILENO, fork_error, strlen(fork_error));
            exit(1);
        }
    }
return 0;
}
