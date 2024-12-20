#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define DEFAULT_BLOCKSIZE 512
#define MAX_BLOCKSIZE 65464
#define MIN_BLOCKSIZE 8
#define TFTP_PORT "69"

void send_request(int sockfd, struct sockaddr_in *server_addr, const char *filename, int blocksize) {
    char buffer[DEFAULT_BLOCKSIZE];
    int len = snprintf(buffer, sizeof(buffer), "\0\1%s\0octet\0blksize\0%d\0", filename, blocksize);

    if (len < 0 || len >= DEFAULT_BLOCKSIZE) {
        fprintf(stderr, "Error: buffer overflow in send_request.\n");
        exit(EXIT_FAILURE);
    } int sent_bytes = sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (sent_bytes < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    printf("Read request sent for file: %s with blocksize: %d\n", filename, blocksize);
}

void receive_multiple_packets(int sockfd, struct sockaddr_in *server_addr, FILE *file, int blocksize) {
    char *buffer = malloc(blocksize + 4); // Allocate memory based on blocksize
    if (!buffer) {
fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(*server_addr);
    int block_num = 1;

    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, blocksize + 4, 0, (struct sockaddr *)server_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");  free(buffer);
            exit(EXIT_FAILURE);
        }

        if (buffer[1] == 3) { // DATA opcode
            int received_block_num = (buffer[2] << 8) | buffer[3];
            if (received_block_num == block_num) {
                fwrite(buffer + 4, 1, bytes_received - 4, file);
                printf("Received data block %d.\n", block_num);

                // Send ACK
                char ack[4] = {0, 4, buffer[2], buffer[3]}; sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, addr_len);
                printf("Acknowledgment sent for block %d.\n", block_num);

                block_num++;
            }
        }

        if (bytes_received < blocksize + 4) { // Last packet
            break;
        }
    }

    free(buffer);
    printf("File transfer complete.\n");}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <server> <filename> [blocksize]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server = argv[1];
    const char *filename = argv[2];
    int blocksize = (argc == 4) ? atoi(argv[3]) : DEFAULT_BLOCKSIZE; if (blocksize < MIN_BLOCKSIZE || blocksize > MAX_BLOCKSIZE) {
        fprintf(stderr, "Invalid blocksize. Must be between %d and %d.\n", MIN_BLOCKSIZE, MAX_BLOCKSIZE);
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; int status = getaddrinfo(server, TFTP_PORT, &hints, &res);
    if (status != 0 || res == NULL) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }struct sockaddr_in *server_addr = (struct sockaddr_in *)res->ai_addr;

    send_request(sockfd, server_addr, filename, blocksize);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        close(sockfd);
        return EXIT_FAILURE;
    }
receive_multiple_packets(sockfd, server_addr, file, blocksize);

    fclose(file);
    freeaddrinfo(res);
    close(sockfd);
return EXIT_SUCCESS;
}
