//
// Created by axelzucho on 15/04/19.
//

#ifndef ERROR_CORRECTION_FILEOPERATIONS_H
#define ERROR_CORRECTION_FILEOPERATIONS_H

typedef struct file_part_s {
    size_t bit_amount;
    u_int32_t entire_crc;
    unsigned char *buffer;
} file_part;

void read_file(char *filename, unsigned char **buffer, size_t *file_length);

void divide_buffer(unsigned char *buffer, file_part **all_parts, int server_amount, size_t file_length);

void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length);

void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file);

#endif //ERROR_CORRECTION_FILEOPERATIONS_H
