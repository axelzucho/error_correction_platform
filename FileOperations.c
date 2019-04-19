//
// Created by axelzucho on 15/04/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "libcrc-2.0/include/checksum.h"
#include "FileOperations.h"

void divide_buffer(unsigned char *buffer, unsigned char *parity, file_part **all_parts, int server_amount,
                   size_t file_length) {
    *all_parts = malloc(server_amount * sizeof(file_part));
    u_int32_t crc = crc_32(buffer, file_length);
    int parity_size = (int) ceil((double) file_length / server_amount) + 1;
    for (int i = 0; i < server_amount; ++i) {
        (*all_parts)[i].entire_crc = crc;
        (*all_parts)[i].buffer = calloc(file_length / server_amount + 2, sizeof(unsigned char));
        (*all_parts)[i].bit_amount = (file_length * 8) / server_amount + (i < file_length % server_amount);

        if (i < 2) {
            (*all_parts)[i].parity_size = parity_size;
            (*all_parts)[i].parity_file = malloc(parity_size * sizeof(unsigned char));
            strncpy((char *) (*all_parts)[i].parity_file, (char *) parity, parity_size);
            (*all_parts)[i].parity_file[parity_size] = '\0';
        } else {
            (*all_parts)[i].parity_size = 0;
            (*all_parts)[i].parity_file = NULL;
        }
    }

    for (int i = 0; i < file_length * 8; ++i) {
        if (i / server_amount > (*all_parts)[i % server_amount].bit_amount) {
            continue;
        }
        bool current_val = (bool) (buffer[i / 8] & (1 << (7 - i % 8)));
        int shift_amount = 7 - (i / server_amount) % 8;

        (*all_parts)[i % server_amount].buffer[i / (server_amount * 8)] |= current_val << shift_amount;
    }

    for (int i = 0; i < server_amount; i++) {
        (*all_parts)[i].buffer[file_length / (server_amount) + 1] = '\0';
    }
}

void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length) {
    for (int i = 0; i < file_length * 8; ++i) {
        if (i / server_amount > all_parts[i % server_amount].bit_amount) {
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

void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file) {
    *parity_file = calloc((size_t) ceil((double) file_length / server_amount) + 1, sizeof(unsigned char));
    bool current_value = false;

    for (int i = 0; i < file_length * 8; i++) {
        if (buffer[i / 8] & (1 << (7 - i % 8))) {
            current_value = !current_value;
        }
        if (i % server_amount == server_amount - 1) {
            int shift_value = 7 - (i / server_amount) % 8;
            (*parity_file)[i / (8 * server_amount)] |= current_value << shift_value;
            current_value = false;
        }
    }

    if (file_length % server_amount != server_amount - 1) {
        int final_shift = 7 - (int) (file_length * 8) / server_amount % 8;
        (*parity_file)[file_length / server_amount] |= current_value << final_shift;
    }
}

void loose_bits(file_part *part_to_loose) {
    memset(part_to_loose->buffer, 0, part_to_loose->bit_amount / 8 + 1);
}

void recover_part(file_part *all_parts, int server_amount, int part_to_recover, unsigned char *parity_file) {
    for (int i = 0; i < all_parts[0].bit_amount; i++) {
        bool current_value = false;
        for (int j = 0; j < server_amount; j++) {
            if (all_parts[j].bit_amount <= i) continue;
            if (all_parts[j].buffer[i / 8] & (1 << (7 - i % 8))) {
                current_value = !current_value;
            }
        }
        bool current_parity = (bool) (parity_file[i / 8] & 1 << (7 - i % 8));
        if (current_parity != current_value && all_parts[part_to_recover].bit_amount > i) {
            all_parts[part_to_recover].buffer[i / 8] |= 1 << (7 - i % 8);
        }
    }
}

void print_descriptive_buffer(file_part *part) {
    for (int i = 0; i < part->bit_amount / 8; ++i) {
        printf("%d ", (int) (part->buffer[i]));
    }
    printf("\n");
}
