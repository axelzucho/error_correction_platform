//
// Created by axelzucho on 19/04/19.
//

#include "FileTransmission.h"
#include "../tools.h"
#include "sockets.h"

void receive_file(int connection_fd, file_part *part){
    char buffer[MAX_STR_LEN];
    recvString(connection_fd, buffer, MAX_STR_LEN);

    int bit_amount;
    sscanf(buffer, "%d", &bit_amount);
    part->bit_amount = (size_t)bit_amount;

    recvString(connection_fd, buffer, MAX_STR_LEN);
    unsigned char *buffer_to_pass = (unsigned char*)buffer;
    part->buffer = malloc(strlen(buffer) * sizeof(unsigned char));
    for(int i = 0; i < strlen(buffer); ++i){
        part->buffer[i] = buffer_to_pass[i];
    }
}

void single_server(){
    file_part* part = malloc(sizeof(file_part));
    int connection_fd = connectSocket("localhost", "9900");
    receive_file(connection_fd, part);
    char buffer[MAX_STR_LEN];
    do{
        recvString(connection_fd, buffer, 100);
    } while(perform_action(buffer, connection_fd, part));
}

void create_all_servers(int *connection_fds, int server_amount){
    int main_fd = initServer("9900", server_amount);
    pid_t new_pid;
    for(int i = 0; i < server_amount; i++){
        new_pid = fork();
        if(new_pid == 0){
            close(main_fd);
            single_server();
            exit(EXIT_SUCCESS);
        } else {
            int client_fd = accept(main_fd, NULL, NULL);
            connection_fds[i] = client_fd;
        }
    }

}

void send_single_part(int connection_fd, file_part * part){
    char int_buff[100];
    sprintf(int_buff, "%d", (int)(part->bit_amount));
    sendString(connection_fd, int_buff, (int)strlen(int_buff));
    sendString(connection_fd, part->buffer, MAX_STR_LEN);
}

void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts){
    for(int i = 0; i < server_amount; ++i){
        send_single_part(connection_fds[i], &all_parts[i]);
    }
}

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts){
    for (int i = 0; i < server_amount; i++){
        sendString(connection_fds[i], (void *)SEND_PARTS_STR, (int)strlen(SEND_PARTS_STR) + 1);
    }
    for (int i = 0; i < server_amount; i++) {
        receive_file(connection_fds[i], &all_parts[i]);
    }
}

bool perform_action(char *buffer, int connection_fd, file_part *part){
    if(strcmp(buffer, SEND_PARTS_STR) == 0){
        send_single_part(connection_fd, part);
        return true;
    }
    return true;
}
