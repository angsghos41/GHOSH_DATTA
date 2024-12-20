#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#define DEFAULT_BLOCKSIZE 512
#define MAX_BLOCKSIZE 65464
#define MIN_BLOCKSIZE 8
#define TFTP_PORT "69"
void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void send_request(int sockfd, struct sockaddr_in *server_addr, const char *filename, int blocksize) {
    char buffer[DEFAULT_BLOCKSIZE];
    int len = snprintf(buffer, sizeof(buffer), "\0\1%s\0octet\0blksize\0%d\0", filename, blocksize);

    if (len < 0 || len >= DEFAULT_BLOCKSIZE) {
        write_message("Error: buffer overflow in send_request.\n");
        exit(EXIT_FAILURE);
    }
int sent_bytes = sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        write_message("Error: Failed to send request.\n");
        exit(EXIT_FAILURE);
    }

    write_message("Request sent successfully.\n");
}

void receive_multiple_packets(int sockfd, struct sockaddr_in *server_addr, int fd, int blocksize) {
    char *buffer = malloc(blocksize + 4); // Allocate memory based on blocksize
    if (!buffer) {
 write_message("Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(*server_addr);
    int block_num = 1;

    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, blocksize + 4, 0, (struct sockaddr *)server_addr, &addr_len);
        if (bytes_received < 0) {
            write_message("Error: Failed to receive data.\n");
            free(buffer);
exit(EXIT_FAILURE);
        }

        if (buffer[1] == 3) { // DATA opcode
            int received_block_num = (buffer[2] << 8) | buffer[3];
            if (received_block_num == block_num) {
                if (write(fd, buffer + 4, bytes_received - 4) < 0) {
                    write_message("Error: Failed to write to file.\n");
                    free(buffer);
                    exit(EXIT_FAILURE);
                }
char ack[4] = {0, 4, buffer[2], buffer[3]};
                sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, addr_len);

                block_num++;
            }
        }

        if (bytes_received < blocksize + 4) { // Last packet
            break;
        }
    }

 free(buffer);
    write_message("File transfer complete.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        write_message("Usage: gettftp <server> <filename> [blocksize]\n");
        return EXIT_FAILURE;
    }
  const char *server = argv[1];
    const char *filename = argv[2];
    int blocksize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BLOCKSIZE;

    if (blocksize < MIN_BLOCKSIZE || blocksize > MAX_BLOCKSIZE) {
        write_message("Invalid blocksize. Must be between 8 and 65464.\n");
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(server, TFTP_PORT, &hints, &res);
    if (status != 0 || res == NULL) {
        write_message("Error: Failed to resolve server address.\n");
        return EXIT_FAILURE;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
  write_message("Error: Failed to create socket.\n");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    struct sockaddr_in *server_addr = (struct sockaddr_in *)res->ai_addr;

    send_request(sockfd, server_addr, filename, blocksize);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
 write_message("Error: Failed to open file for writing.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    receive_multiple_packets(sockfd, server_addr, fd, blocksize);

    close(fd);
    freeaddrinfo(res);
    close(sockfd);
return EXIT_SUCCESS;
}
