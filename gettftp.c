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

// Function to create the TFTP request packet
void create_rrq_packet(char *filename, char *packet) {
    packet[0] = 0;  // Zero byte for TFTP
    packet[1] = RRQ; // Read Request
    strcpy(&packet[2], filename);

memset(&packet[2 + strlen(filename)], 0, 1); // Null-terminate the filename
    strcpy(&packet[3 + strlen(filename)], "octet"); // Mode: octet (binary)
    memset(&packet[3 + strlen(filename) + 5], 0, 1); // Null-terminate "octet"
}

// Function to receive data from the TFTP server
void receive_file(int sockfd, struct sockaddr_in server_addr, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "wb");
    if (!file) {
  perror("Failed to open file for writing");
        return;
    }

    int block_num = 1;
    while (1) {
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len < 0) {
            perror("Failed to receive data");
            break;
  }

        // Check if it's a data packet (opcode 3)
        if (buffer[1] == DATA) {
            // Write the data to the file
            fwrite(&buffer[4], 1, len - 4, file);

            // Send an acknowledgment for the received block
            buffer[0] = 0;
            buffer[1] = ACK;
            buffer[2] = (block_num >> 8) & 0xFF;
            buffer[3] = block_num & 0xFF;
            sendto(sockfd, buffer, 4, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

   if (len < BUFFER_SIZE) {
                break; // Last block received
            }

            block_num++;
        }
    }

    fclose(file);
}

// Main function for downloading a file
int main(int argc, char *argv[]) {
 if (argc != 3) {
        fprintf(stderr, "Usage: %s <server> <filename>\n", argv[0]);
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
// Send RRQ packet
    char packet[BUFFER_SIZE];
    create_rrq_packet(filename, packet);
    sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Receive the file from the TFTP server
    receive_file(sockfd, server_addr, filename);

    close(sockfd);
return 0;
}
