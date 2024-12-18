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
    // Build a TFTP Write Request (WRQ) packet
    int len = snprintf(buffer, sizeof(buffer), "\0\2%s\0octet\0", filename);
    if (sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
    printf("Write request sent for file: %s\n", filename);
}
// Function to send data blocks (file contents) to the server
void send_file(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int block_num = 1;
    socklen_t addr_len = sizeof(*server_addr);

  // Send data in blocks to the server
    while (1) {
        int bytes_read = fread(buffer + 4, 1, BUFFER_SIZE - 4, file);
        if (bytes_read == 0) {
            break; // End of file
        }

        // Set up the block number and data in the packet
        buffer[0] = 0; // TFTP data packet opcode
        buffer[1] = 3; // Data packet type
        buffer[2] = (block_num >> 8) & 0xFF; // High byte of block number
   buffer[3] = block_num & 0xFF; // Low byte of block number

        // Send the data block to the server
        if (sendto(sockfd, buffer, bytes_read + 4, 0, (struct sockaddr *)server_addr, addr_len) < 0) {
            perror("sendto");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Wait for the acknowledgment (ACK) from the server
        char ack[4];
    if (recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, &addr_len) < 0) {
            perror("recvfrom");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Check if the acknowledgment block number matches the sent block
        if (ack[1] != 4 || ack[2] != buffer[2] || ack[3] != buffer[3]) {
            fprintf(stderr, "Error: Unexpected ACK\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

   printf("Block %d sent successfully.\n", block_num);
        block_num++;
    }

    fclose(file);
    printf("File uploaded successfully.\n");
}

// Main function to set up the connection and initiate the file upload
int main(int argc, char *argv[]) {
    if (argc != 3) {
  fprintf(stderr, "Usage: %s <server> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server = argv[1];  // Server address
    const char *filename = argv[2]; // File to upload

    // Prepare to resolve the server's address
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
 hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    // Resolve the server's address using getaddrinfo
    int status = getaddrinfo(server, TFTP_PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

 // Print the resolved server address
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(res->ai_family, &((struct sockaddr_in *)res->ai_addr)->sin_addr, ipstr, sizeof(ipstr));
    printf("Resolved server address: %s\n", ipstr);

    // Create a UDP socket for communication
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return EXIT_FAILURE;
  }

    // Cast to sockaddr_in for convenience
    struct sockaddr_in *server_addr = (struct sockaddr_in *)res->ai_addr;

    // Send the Write Request (WRQ) to the server
    send_request(sockfd, server_addr, filename);

    // Upload the file in blocks
    send_file(sockfd, server_addr, filename);

    // Cleanup
    freeaddrinfo(res);
    close(sockfd);
return EXIT_SUCCESS;
}
