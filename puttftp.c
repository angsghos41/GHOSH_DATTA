#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TFTP_PORT 69
#define BUFFER_SIZE 512

// TFTP opcodes
#define RRQ 1   // Read request
#define WRQ 2   // Write request
#define DATA 3  // Data packet
#define ACK 4   // Acknowledgement
#define ERROR 5 // Error packet
// Function to create the TFTP write request packet
void create_wrq_packet(char *filename, char *packet) {
    packet[0] = 0;  // Zero byte for TFTP
    packet[1] = WRQ; // Write Request
    strcpy(&packet[2], filename);
    memset(&packet[2 + strlen(filename)], 0, 1); // Null-terminate the filename
    strcpy(&packet[3 + strlen(filename)], "octet"); // Mode: octet (binary)
    memset(&packet[3 + strlen(filename) + 5], 0, 1); // Null-terminate "octet"
}

// Function to send file to the TFTP server
void send_file(int sockfd, struct sockaddr_in server_addr, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file for reading");
        return;
    }

    int block_num = 1;
    while (1) {
  int bytes_read = fread(&buffer[4], 1, BUFFER_SIZE - 4, file);
        if (bytes_read <= 0) {
            break; // End of file
        }

        // Prepare data packet (opcode 3)
        buffer[0] = 0;
        buffer[1] = DATA;
        buffer[2] = (block_num >> 8) & 0xFF;
        buffer[3] = block_num & 0xFF;
        // Send data packet
        sendto(sockfd, buffer, bytes_read + 4, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        // Wait for acknowledgment
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len < 0) {
            perror("Failed to receive acknowledgment");
            break;
        }
        // Check if acknowledgment is for the correct block
        if (buffer[1] == ACK && buffer[2] == (block_num >> 8) && buffer[3] == (block_num & 0xFF)) {
            block_num++;
        }
    }

    fclose(file);
}

// Main function for uploading a file
int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <filename>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    char *filename = argv[2];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
       perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TFTP_PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        return 1;
    }
// Send WRQ packet
    char packet[BUFFER_SIZE];
    create_wrq_packet(filename, packet);
    sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Send the file to the TFTP server
    send_file(sockfd, server_addr, filename);

    close(sockfd);
return 0;
}
