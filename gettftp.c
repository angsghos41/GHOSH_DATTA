#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 516
#define TFTP_PORT "69" // TFTP default port
// Function to send a request to the server
void send_request(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    char buffer[BUFFER_SIZE];
    // Build a TFTP Read Request (RRQ) packet
    int len = snprintf(buffer, sizeof(buffer), "\0\1%s\0octet\0", filename);
    if (sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
    printf("Read request sent for file: %s\n", filename);
}
// Function to receive data blocks (file contents) from the server
void receive_file(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(*server_addr);
    int block_num = 1;

    // Receive data blocks from the server
 while (1) {
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        if (buffer[1] == 3) { // Data packet
            int received_block = (buffer[2] << 8) + buffer[3];
            if (received_block == block_num) {
   fwrite(buffer + 4, 1, bytes_received - 4, file);
                block_num++;
            }
            char ack[4] = {0, 4, buffer[2], buffer[3]};
            sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, addr_len);
        }

        if (bytes_received < BUFFER_SIZE) break; // Last packet
    }

    fclose(file);
    printf("File received successfully.\n");
}
// Main function to set up the connection and initiate the file download
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server = argv[1];  // Server address
    const char *filename = argv[2]; // File to download

    // Prepare to resolve the server's address
    struct addrinfo hints, *res;
memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    // Resolve the server's address using getaddrinfo
    int status = getaddrinfo(server, TFTP_PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

 // Create a UDP socket for communication
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }

    // Cast to sockaddr_in for convenience
    struct sockaddr_in *server_addr = (struct sockaddr_in *)res->ai_addr;

    // Send the Read Request (RRQ) to the server
send_request(sockfd, server_addr, filename);

    // Receive the file in blocks
    receive_file(sockfd, server_addr, filename);

    // Cleanup
    freeaddrinfo(res);
    close(sockfd);
return EXIT_SUCCESS;
}
