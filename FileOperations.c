//
// Created by axelzucho on 15/04/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "libcrc-2.0/include/checksum.h"
#include "FileOperations.h"

void divide_buffer(unsigned char *buffer, file_part **all_parts, int server_amount, size_t file_length) {
    *all_parts = malloc(server_amount * sizeof(file_part));
    u_int32_t crc = crc_32(buffer, file_length);

    for (int i = 0; i < server_amount; ++i) {
        (*all_parts)[i].entire_crc = crc;
        (*all_parts)[i].buffer = calloc(file_length / (server_amount * 8) + 1, sizeof(unsigned char));
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
    *parity_file = calloc(file_length/3 + 1, sizeof(unsigned char));
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

    int final_shift = 7 - (int)(file_length/server_amount) % 8;
    (*parity_file)[file_length/(8*server_amount)] |= current_value << final_shift;

}
