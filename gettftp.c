#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#define DEFAULT_BLOCK_SIZE 512
#define MAX_BLOCK_SIZE 65464
#define MIN_BLOCK_SIZE 8
#define TFTP_PORT "69"
#define TIMEOUT_SEC 2
#define MAX_RETRIES 5
void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void send_request(int sockfd, struct sockaddr_in *server_addr, const char *filename, int blocksize) {
    char buffer[516];
    int len = snprintf(buffer, sizeof(buffer), "\0\1%s\0octet\0blksize\0%d\0", filename, blocksize);
    if (len < 0 || len >= sizeof(buffer)) {
        write_message("Error: Buffer overflow in send_request.\n");
        exit(EXIT_FAILURE);
    }
    if (sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

int receive_packet(int sockfd, struct sockaddr_in *server_addr, char *buffer, size_t buffer_size) {
    socklen_t addr_len = sizeof(*server_addr);
    return recvfrom(sockfd, buffer, buffer_size, 0, (struct sockaddr *)server_addr, &addr_len);
}
void send_ack(int sockfd, struct sockaddr_in *server_addr, int block_num) {
    char ack[4] = {0, 4, (block_num >> 8) & 0xFF, block_num & 0xFF};
    if (sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

void handle_error_packet(const char *buffer, ssize_t bytes_received) {
    if (bytes_received >= 4 && buffer[1] == 5) {
        int error_code = (buffer[2] << 8) | buffer[3];
        write_message("Error packet received: ");
        write_message(buffer + 4); // Error message starts at buffer[4]
        write_message("\n");
        exit(EXIT_FAILURE);
    }
}

void transfer_file(int sockfd, struct sockaddr_in *server_addr, int fd, int blocksize) {
    char buffer[blocksize + 4];
    int block_num = 1, retries = 0;
    ssize_t bytes_received;
    struct timeval timeout = {TIMEOUT_SEC, 0};
    fd_set read_fds;

    while (1) {
        // Wait for the data packet or timeout
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        int sel = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (sel < 0) {
           perror("select");
            exit(EXIT_FAILURE);
        } else if (sel == 0) { // Timeout occurred
            if (retries >= MAX_RETRIES) {
                write_message("Error: Maximum retries reached. Exiting.\n");
                exit(EXIT_FAILURE);
            }
            write_message("Timeout occurred. Resending ACK...\n");
            send_ack(sockfd, server_addr, block_num - 1);
            retries++;
            continue;
        }

        // Receive the data packet
        bytes_received = receive_packet(sockfd, server_addr, buffer, sizeof(buffer));
        if (bytes_received < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        handle_error_packet(buffer, bytes_received); // Check for error packet
        if (buffer[1] == 3) { // Data packet
            int received_block_num = (buffer[2] << 8) | buffer[3];
            if (received_block_num == block_num) {
                retries = 0; // Reset retries on successful packet reception
                write(fd, buffer + 4, bytes_received - 4); // Write data to file
                send_ack(sockfd, server_addr, block_num);  // Send acknowledgment
                block_num++;
            }
        }
       // End of file if the received packet is smaller than the blocksize
        if (bytes_received < blocksize + 4) {
            break;
        }
    }

    write_message("File transfer complete.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        write_message("Usage: gettftp <server> <filename> [blocksize]\n");
        return EXIT_FAILURE;
    }

    const char *server = argv[1];
    const char *filename = argv[2];
    int blocksize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BLOCK_SIZE;

    if (blocksize < MIN_BLOCK_SIZE || blocksize > MAX_BLOCK_SIZE) {
        write_message("Error: Blocksize must be between 8 and 65464.\n");
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server, TFTP_PORT, &hints, &res) != 0 || res == NULL) {
        write_message("Error: Failed to resolve server address.\n");

        return EXIT_FAILURE;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    struct sockaddr_in *server_addr = (struct sockaddr_in *)res->ai_addr;
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        freeaddrinfo(res);
        close(sockfd);
        return EXIT_FAILURE;
    }

    send_request(sockfd, server_addr, filename, blocksize);
    transfer_file(sockfd, server_addr, fd, blocksize);
   close(fd);
    freeaddrinfo(res);
    close(sockfd);
return EXIT_SUCCESS;
}
