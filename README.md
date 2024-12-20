# TFTP Client Implementation

## Overview
This project implements two TFTP (Trivial File Transfer Protocol) clients for file transfer between a client and a server, adhering to the TFTP specifications. The clients are:

1. `gettftp`: Downloads a file from the server.
2. `puttftp`: Uploads a file to the server.

## Features
The project includes the following functionalities:

1. **Command-Line Arguments**: Both clients take the server address and filename as command-line arguments.
2. **Address Resolution**: Uses `getaddrinfo` to resolve the server’s address.
3. **Socket Reservation**: Reserves a connection socket to communicate with the server.
4. **File Transfer Operations**:
    - For `gettftp` (Download):
        - Send a Read Request (RRQ).
        - Handle single-packet and multi-packet file downloads with acknowledgments (ACK).
    - For `puttftp` (Upload):
        - Send a Write Request (WRQ).
        - Handle single-packet and multi-packet file uploads with acknowledgments (ACK).
5. **Blocksize Option**: Implements the blocksize option to improve transfer efficiency.
6. **Optimal Blocksize**: Allows the user to specify the blocksize and optimizes it for transfer speed.
7. **Error Handling**: Handles packet loss and processes error packets (ERR).
8. **Timeout and Retransmissions**: Includes a timeout mechanism with retries to ensure reliable transfers.

## Usage

### Compilation
Compile the programs using the following command:
```bash
gcc -o gettftp gettftp.c
gcc -o puttftp puttftp.c
```

### Running the Clients

#### Download a File
Use the `gettftp` client to download a file from the server:
```bash
./gettftp <server> <file> [blocksize]
```
- `<server>`: IP address or hostname of the TFTP server.
- `<file>`: Name of the file to download.
- `[blocksize]` (optional): Blocksize for the transfer (default is 512).

#### Upload a File
Use the `puttftp` client to upload a file to the server:
```bash
./puttftp <server> <file> [blocksize]
```
- `<server>`: IP address or hostname of the TFTP server.
- `<file>`: Name of the file to upload.
- `[blocksize]` (optional): Blocksize for the transfer (default is 512).

## Implementation Details

### Features Implemented in Order

1. **Command-Line Arguments**: Both clients accept server and file information as arguments for flexibility.

2. **Address Resolution**: Used `getaddrinfo` to resolve the server’s address for UDP communication.

3. **Socket Reservation**: A UDP socket is created and bound to the server’s address.

4. **File Transfer Operations**:
    - `gettftp`:
        - Constructs a valid RRQ packet.
        - Handles single and multiple data packets, sending acknowledgments for each.
    - `puttftp`:
        - Constructs a valid WRQ packet.
        - Handles single and multiple data packets, waiting for acknowledgments from the server.

5. **Blocksize Option**: Implements blocksize negotiation using the TFTP options.

6. **Optimal Blocksize**: Allows user-specified blocksize to optimize transfer speed.

7. **Error Handling**:
    - Retransmits packets on timeout.
    - Processes error packets (ERR) and gracefully exits on fatal errors.

8. **Timeout and Retransmissions**:
    - Implements a timeout mechanism using `select()`.
    - Retransmits packets up to a maximum number of retries to ensure reliability.

## Example

### Download Example
```bash
./gettftp 127.1.1.0 motd.txt 
```
This command downloads the file `example.txt` from the server at `192.168.1.10` with a blocksize of 1024 bytes.

### Upload Example
```bash
./puttftp 127.1.1.0 motd.txt 
```
This command uploads the file `upload.txt` to the server at `192.168.1.10` with a blocksize of 1024 bytes.

## Notes
- The default blocksize is 512 bytes as per the TFTP standard. You can override this with the optional blocksize argument.
- Ensure the server is configured to allow TFTP transfers and the necessary files exist for `gettftp` operations.
- Error and timeout handling is implemented to make transfers robust against network issues.

