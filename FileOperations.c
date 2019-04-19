//
// Created by axelzucho on 15/04/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wait.h>

#include "libcrc-2.0/include/checksum.h"
#include "FileOperations.h"
#include "sockets/sockets.h"
#include "tools.h"

#define MAX_STR_LEN 100000

void divide_buffer(unsigned char *buffer, file_part **all_parts, int server_amount, size_t file_length) {
    *all_parts = malloc(server_amount * sizeof(file_part));
    u_int32_t crc = crc_32(buffer, file_length);

    for (int i = 0; i < server_amount; ++i) {
        (*all_parts)[i].entire_crc = crc;
        (*all_parts)[i].buffer = calloc(file_length/server_amount + 1, sizeof(unsigned char));
        (*all_parts)[i].bit_amount = (file_length * 8) / server_amount + 1;
    }

    for (int i = 0; i < file_length * 8; ++i) {
        if(i / server_amount > (*all_parts)[i % server_amount].bit_amount){
            continue;
        }
        bool current_val = (bool) (buffer[i / 8] & (1 << (7 - i % 8)));
        int shift_amount = 7 - (i / server_amount) % 8;

        (*all_parts)[i % server_amount].buffer[i / (server_amount * 8)] |= current_val << shift_amount;
    }
}

void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length) {
    for (int i = 0; i < file_length * 8; ++i) {
        if(i / server_amount > all_parts[i % server_amount].bit_amount){
            continue;
        }
        bool current_val = (bool) (all_parts[i % server_amount].buffer[i / (server_amount * 8)] &
                                   (1 << (7 - (i / server_amount) % 8)));
        int shift_amount = 7 - (i % 8);

        buffer[i / 8] |= current_val << shift_amount;
    }
}

void read_file(char *filename, unsigned char **buffer, size_t *file_length) {
    FILE *file;

    file = fopen(filename, "rb");

    if (file == NULL) {
        printf("Can't open file\n");
    }

    fseek(file, 0, SEEK_END);
    *file_length = (size_t) ftell(file);
    rewind(file);

    *buffer = malloc((*file_length + 1) * sizeof(unsigned char));
    fread(*buffer, *file_length, 1, file);

    fclose(file);
}

void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file){
    *parity_file = calloc(file_length/server_amount + 2, sizeof(unsigned char));
    bool current_value = false;

    for(int i = 0; i < file_length * 8; i++){
        if(buffer[i / 8] & (1 << (7 - i % 8))){
           current_value = !current_value;
        }
        if(i % server_amount == server_amount - 1){
            int shift_value = 7 - (i / server_amount) % 8;
            (*parity_file)[i/(8*server_amount)] |= current_value << shift_value;
            current_value = false;
        }
    }

    if(file_length % server_amount != server_amount - 1){
        int final_shift =  7 - (int)(file_length*8)/server_amount % 8;
        (*parity_file)[file_length/server_amount] |= current_value << final_shift;
    }
}

void loose_bits(file_part *part_to_loose){
    memset(part_to_loose->buffer, 0, part_to_loose->bit_amount/8 + 1);
}

void recover_part(file_part *all_parts, int server_amount, int part_to_recover, unsigned char *parity_file){
    for(int i = 0; i < all_parts[0].bit_amount; i++){
        bool current_value = false;
        for(int j = 0; j < server_amount; j++){
            if(all_parts[j].bit_amount <= i) continue;
            if(all_parts[j].buffer[i/8] & (1 << (7 - i % 8))) {
                current_value = !current_value;
            }
        }
        bool current_parity = (bool)(parity_file[i/8] & 1 << (7 - i % 8));
        if(current_parity != current_value && all_parts[part_to_recover].bit_amount > i){
            all_parts[part_to_recover].buffer[i/8] |= 1 << (7 - i % 8);
        }
    }
}

void print_descriptive_buffer(file_part * part){
    for(int i = 0; i < part->bit_amount/8; ++i){
        printf("%d ", (int)(part->buffer[i]));
    }
    printf("\n");
}

void receive_file(int connection_fd, file_part *part){
    char *buffer = malloc(MAX_STR_LEN);
    recvString(connection_fd, buffer, MAX_STR_LEN);
    part->bit_amount = ((file_part *)buffer)->bit_amount;
    part->entire_crc = ((file_part *)buffer)->entire_crc;
    part->buffer = ((file_part *)buffer)->buffer;
    part->parity_file = ((file_part *)buffer)->parity_file;
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

void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts){
    for(int i = 0; i < server_amount; ++i){
        sendString(connection_fds[i], (void*)&all_parts[i], sizeof(all_parts));
    }
}

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts){
    for (int i = 0; i < server_amount; i++){
        sendString(connection_fds[i], (void *)SEND_PARTS_STR, (int)strlen(SEND_PARTS_STR) + 1);
    }
    for (int i = 0; i < server_amount; i++) {
        char *buffer = malloc(MAX_STR_LEN);
        recvString(connection_fds[i], buffer, MAX_STR_LEN);
        all_parts[i] = *((file_part *)buffer);
    }
}

bool perform_action(char *buffer, int connection_fd, file_part *part){
    if(strcmp(buffer, SEND_PARTS_STR) == 0){
        sendString(connection_fd, (void *)part, sizeof(file_part *));
        return true;
    }
    return false;
}