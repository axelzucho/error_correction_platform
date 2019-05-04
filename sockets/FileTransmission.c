//
// Created by axelzucho on 19/04/19.
//

#include <math.h>
#include "FileTransmission.h"
#include "../tools.h"
#include "sockets.h"

void send_file(int connection_fd, unsigned char *file, size_t file_length);

void receive_file(int connection_fd, unsigned char **file, size_t file_length);

void receive_part(int connection_fd, file_part *part);

void send_single_part(int connection_fd, file_part *part);

void single_server();

bool perform_action(char *buffer, int connection_fd, file_part *part);

// Sends a big file to a server.
void send_file(int connection_fd, unsigned char *file, size_t file_length) {
    // The total of bytes sent.
    size_t total = 0;
    // While there are still bytes to send.
    while (total < file_length) {
        // Try to send the total of bytes left.
        ssize_t partial = send(connection_fd, file + total, file_length - total, 0);
        if (partial == -1) {
            perror("SEND: ");
        }
        // Add to the bytes sent so far.
        total += partial;
    }
}

// Receive a big file form a server.
void receive_file(int connection_fd, unsigned char **file, size_t file_length) {
    // The total of bytes received
    size_t total = 0;
    // While there are still bytes to receive.
    while (total < file_length) {
        // Try to receive the total of bytes left.
        ssize_t partial = recv(connection_fd, *file + total, file_length - total, 0);
        if (partial == -1) {
            perror("RECV: ");
        }
        // Add to the total amount of bytes received so far.
        total += partial;
    }
}

// Receive an entire part to a server.
void receive_part(int connection_fd, file_part *part) {
    char buffer[MAX_STR_LEN];
    // Receive the amount of bits for the specific part.
    recvString(connection_fd, buffer, MAX_STR_LEN);
    sendString(connection_fd, RECEIVED_MESSAGE, strlen(RECEIVED_MESSAGE));

    sscanf(buffer, "%lu", &part->bit_amount);

    // Calculate the buffer size needed for the part.
    size_t buff_size = (size_t) ceil((double) part->bit_amount / 8);

    // If it has bits, then allocate the buffer.
    if (part->bit_amount > 0) {
        part->buffer = malloc(buff_size * sizeof(unsigned char));
        // Receive the large file.
        receive_file(connection_fd, &(part->buffer), buff_size);
    } else {
        recvString(connection_fd, buffer, MAX_STR_LEN);
        part->buffer = NULL;
    }
    sendString(connection_fd, RECEIVED_MESSAGE, strlen(RECEIVED_MESSAGE));

    // Receive the parity size.
    recvString(connection_fd, buffer, MAX_STR_LEN);
    sendString(connection_fd, RECEIVED_MESSAGE, strlen(RECEIVED_MESSAGE));
    sscanf(buffer, "%lu", &part->parity_size);

    // If it has bits, then receive the file.
    if (part->parity_size > 0) {
        part->parity_file = malloc(part->parity_size * sizeof(unsigned char));
        receive_file(connection_fd, &(part->parity_file), part->parity_size);
    } else {
        recvString(connection_fd, buffer, MAX_STR_LEN);
        part->parity_file = NULL;
    }
    sendString(connection_fd, RECEIVED_MESSAGE, strlen(RECEIVED_MESSAGE));
}

// The loop for a single server.
void single_server() {
    // We need to allocate the memory for the part before receiving it.
    file_part *part = malloc(sizeof(file_part));
    int connection_fd = connectSocket("localhost", "9900");
    // Receive the part.
    receive_part(connection_fd, part);
    char buffer[MAX_STR_LEN];
    do {
        recvString(connection_fd, buffer, MAX_STR_LEN);
        // We keep receiving instructions till we send the part.
    } while (perform_action(buffer, connection_fd, part));

    // Free the allocated memory.
    free(part);
    close(connection_fd);
}

void create_all_servers(int *connection_fds, int server_amount) {
    int main_fd = initServer("9900", server_amount);
    pid_t new_pid;
    // Loop to create all the servers.
    for (int i = 0; i < server_amount; i++) {
        new_pid = fork();
        // If it is the child process.
        if (new_pid == 0) {
            // Close the parent fd and start the server loop.
            close(main_fd);
            single_server();
            exit(EXIT_SUCCESS);
            // If it is the parent process, accept a new connection and add it to the array.
        } else {
            int client_fd = accept(main_fd, NULL, NULL);
            connection_fds[i] = client_fd;
        }
    }

}

// Sends a single part to a specific server.
void send_single_part(int connection_fd, file_part *part) {
    char int_buff[MAX_PARITY_LEN];
    size_t buffer_size = (size_t) ceil((double) part->bit_amount / 8);
    sprintf(int_buff, "%lu", (part->bit_amount));
    // Send the bit amount for the current part.
    sendString(connection_fd, int_buff, strlen(int_buff));
    recvString(connection_fd, int_buff, strlen(RECEIVED_MESSAGE));

    // If it was 0, send the string indicating there is no information to be sent.
    if (part->bit_amount == 0) {
        sendString(connection_fd, NO_INFORMATION, strlen(NO_INFORMATION));
    } else {
        // Send the file otherwise.
        send_file(connection_fd, part->buffer, buffer_size);
    }

    recvString(connection_fd, int_buff, strlen(RECEIVED_MESSAGE));

    char parity_size[MAX_PARITY_LEN];
    sprintf(parity_size, "%ld", part->parity_size);
    // Send the parity size.
    sendString(connection_fd, parity_size, strlen(int_buff));
    recvString(connection_fd, int_buff, strlen(RECEIVED_MESSAGE));

    // Check if we need to send the parity file.
    if (part->parity_size > 0) {
        send_file(connection_fd, part->parity_file, part->parity_size);
        recvString(connection_fd, int_buff, strlen(RECEIVED_MESSAGE));
    } else {
        sendString(connection_fd, NO_INFORMATION, strlen(NO_INFORMATION));
        recvString(connection_fd, int_buff, strlen(RECEIVED_MESSAGE));
    }
}

void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts) {
    // Iterate through all the servers and send each their corresponding part.
    for (int i = 0; i < server_amount; ++i) {
        send_single_part(connection_fds[i], &all_parts[i]);
    }
}

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts) {
    // Iterate through all the servers sending the request for their parts.
    for (int i = 0; i < server_amount; i++) {
        sendString(connection_fds[i], (void *) SEND_PARTS_STR, strlen(SEND_PARTS_STR));
    }
    // Receive each part.
    for (int i = 0; i < server_amount; i++) {
        receive_part(connection_fds[i], &all_parts[i]);
    }
}

// Performs an action and returns if the server should continue existing.
bool perform_action(char *buffer, int connection_fd, file_part *part) {
    // If a part was requested, then send the part.
    if (strcmp(buffer, SEND_PARTS_STR) == 0) {
        send_single_part(connection_fd, part);
        // Return the value to indicate that we should exit the request loop and close the server.
        return false;
        // If a deletion was requested
    } else if (strcmp(buffer, DELETE_PART_STR) == 0) {
        // Delete the part information.
        loose_bits(part);
        // Send the ACK message.
        sendString(connection_fd, RECEIVED_MESSAGE, strlen(RECEIVED_MESSAGE));
        return true;
    }
    // Indicate to continue the loop.
    return true;
}

void send_clear_instruction(int connection_fd) {
    char buffer[MAX_STR_LEN];
    // Send the request for deletion to the specified server.
    sendString(connection_fd, DELETE_PART_STR, strlen(DELETE_PART_STR));
    // Receive the ACK.
    recvString(connection_fd, buffer, MAX_STR_LEN);
}
