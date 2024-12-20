#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#define DEFAULT_BLOCK_SIZE 512
#define MAX_BLOCK_SIZE 65464
#define MIN_BLOCK_SIZE 8
#define TFTP_PORT "69"
void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void send_request(int sockfd, struct sockaddr_in *server_addr, const char *filename, int blocksize) {
    char buffer[516];
    int len = snprintf(buffer, sizeof(buffer), "\0\2%s\0octet\0blksize\0%d\0", filename, blocksize);
    if (len < 0 || len >= sizeof(buffer)) {
        write_message("Error: Buffer overflow in send_request.\n");
        exit(EXIT_FAILURE);
    }
 if (sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

void send_data_packet(int sockfd, struct sockaddr_in *server_addr, int fd, int block_num, int blocksize) {
    char buffer[blocksize + 4];
    ssize_t bytes_read;

    buffer[0] = 0;
    buffer[1] = 3; // Data packet type
    buffer[2] = (block_num >> 8) & 0xFF;
    buffer[3] = block_num & 0xFF;

    bytes_read = read(fd, buffer + 4, blocksize);
    if (bytes_read > 0) {
        if (sendto(sockfd, buffer, bytes_read + 4, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }
}

void receive_ack(int sockfd, struct sockaddr_in *server_addr, int block_num) {
    char buffer[4];
    socklen_t addr_len = sizeof(*server_addr);
    while (1) {
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        if (buffer[1] == 4 && buffer[2] == (block_num >> 8) && buffer[3] == (block_num & 0xFF)) {
            break;
        }
    }
}
void transfer_file(int sockfd, struct sockaddr_in *server_addr, int fd, int blocksize) {
    int block_num = 1;
    while (1) {
        send_data_packet(sockfd, server_addr, fd, block_num, blocksize);
        receive_ack(sockfd, server_addr, block_num);

        block_num++;

        if (lseek(fd, 0, SEEK_CUR) == lseek(fd, 0, SEEK_END)) {
            break;
        }
    }
    write_message("File transfer complete.\n");
}
int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        write_message("Usage: tftp_client <server> <filename> [blocksize]\n");
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

    int fd = open(filename, O_RDONLY);
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
