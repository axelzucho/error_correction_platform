//
// Created by axelzucho on 19/04/19.
//

#include <math.h>
#include "FileTransmission.h"
#include "../tools.h"
#include "sockets.h"

void send_file(int connection_fd, unsigned char *file, size_t file_length){
   size_t total = 0;
   while (total < file_length){
       ssize_t partial = send(connection_fd, file + total, file_length - total, 0);
       if(partial == -1){
           perror("SEND: ");
       }
       total += partial;
   }
}

void receive_file(int connection_fd, unsigned char **file, size_t file_length){
    size_t total = 0;
    while (total < file_length){
        ssize_t partial = recv(connection_fd, *file + total, file_length - total, 0);
        if(partial == -1){
            perror("RECV: ");
        }
        total += partial;
    }
}

void receive_part(int connection_fd, file_part *part) {
    char buffer[MAX_STR_LEN];
    recvString(connection_fd, buffer, MAX_STR_LEN);
    sendString(connection_fd, RECEIVED_MESSAGE, (int) strlen(RECEIVED_MESSAGE));

    int bit_amount;
    sscanf(buffer, "%d", &bit_amount);
    part->bit_amount = (size_t) bit_amount;

    size_t buff_size = (size_t) ceil((double) part->bit_amount / 8);

    if(bit_amount > 0){
        part->buffer = malloc(buff_size * sizeof(unsigned char) + 1);
        receive_file(connection_fd, &(part->buffer), buff_size);
    } else{
        recvString(connection_fd, buffer, MAX_STR_LEN);
        part->buffer = NULL;
    }
    sendString(connection_fd, RECEIVED_MESSAGE, (int) strlen(RECEIVED_MESSAGE));

    recvString(connection_fd, buffer, MAX_STR_LEN);
    sendString(connection_fd, RECEIVED_MESSAGE, (int) strlen(RECEIVED_MESSAGE));
    sscanf(buffer, "%d", &part->parity_size);

    recvString(connection_fd, buffer, MAX_STR_LEN);
    sendString(connection_fd, RECEIVED_MESSAGE, (int) strlen(RECEIVED_MESSAGE));
    if (strcmp(buffer, NO_INFORMATION) == 0) {
        part->parity_file = NULL;
    } else {
        part->parity_file = malloc(part->parity_size * sizeof(unsigned char));
        strcpy((char *) part->parity_file, buffer);
    }
}

void single_server() {
    file_part *part = malloc(sizeof(file_part));
    int connection_fd = connectSocket("localhost", "9900");
    receive_part(connection_fd, part);
    char buffer[MAX_STR_LEN];
    do {
        recvString(connection_fd, buffer, MAX_STR_LEN);
    } while (perform_action(buffer, connection_fd, part));

    free(part);
    close(connection_fd);
}

void create_all_servers(int *connection_fds, int server_amount) {
    int main_fd = initServer("9900", server_amount);
    pid_t new_pid;
    for (int i = 0; i < server_amount; i++) {
        new_pid = fork();
        if (new_pid == 0) {
            close(main_fd);
            single_server();
            exit(EXIT_SUCCESS);
        } else {
            int client_fd = accept(main_fd, NULL, NULL);
            connection_fds[i] = client_fd;
        }
    }

}

void send_single_part(int connection_fd, file_part *part) {
    char int_buff[MAX_PARITY_LEN];
    size_t buffer_size = (size_t) ceil((double) part->bit_amount / 8);
    sprintf(int_buff, "%d", (int) (part->bit_amount));
    sendString(connection_fd, int_buff, (int) strlen(int_buff));
    recvString(connection_fd, int_buff, (int) strlen(RECEIVED_MESSAGE));

    if(part->bit_amount == 0){
        sendString(connection_fd, NO_INFORMATION, (int) strlen(NO_INFORMATION));
    } else {
        send_file(connection_fd, part->buffer, buffer_size);
    }

    recvString(connection_fd, int_buff, (int) strlen(RECEIVED_MESSAGE));

    char parity_size[MAX_PARITY_LEN];
    sprintf(parity_size, "%d", part->parity_size);
    sendString(connection_fd, parity_size, (int) strlen(int_buff));
    recvString(connection_fd, int_buff, (int) strlen(RECEIVED_MESSAGE));

    if (part->parity_size > 0) {
        sendString(connection_fd, (char *) part->parity_file, part->parity_size);
        recvString(connection_fd, int_buff, (int) strlen(RECEIVED_MESSAGE));
    } else {
        sendString(connection_fd, NO_INFORMATION, (int) strlen(NO_INFORMATION));
        recvString(connection_fd, int_buff, (int) strlen(RECEIVED_MESSAGE));
    }
}


void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts) {
    for (int i = 0; i < server_amount; ++i) {
        send_single_part(connection_fds[i], &all_parts[i]);
    }
}

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts) {
    for (int i = 0; i < server_amount; i++) {
        sendString(connection_fds[i], (void *) SEND_PARTS_STR, (int) strlen(SEND_PARTS_STR) + 1);
    }
    for (int i = 0; i < server_amount; i++) {
        receive_part(connection_fds[i], &all_parts[i]);
    }
}

bool perform_action(char *buffer, int connection_fd, file_part *part) {
    if (strcmp(buffer, SEND_PARTS_STR) == 0) {
        send_single_part(connection_fd, part);
        return false;
    } else if (strcmp(buffer, DELETE_PART_STR) == 0) {
        loose_bits(part);
        sendString(connection_fd, RECEIVED_MESSAGE, (int) strlen(RECEIVED_MESSAGE));
        return true;
    }
    return true;
}

void send_clear_instruction(int connection_fd){
    char buffer[MAX_STR_LEN];
    sendString(connection_fd, DELETE_PART_STR, (int)strlen(DELETE_PART_STR));
    recvString(connection_fd, buffer, MAX_STR_LEN);
}
