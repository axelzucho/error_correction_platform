//
// Created by axelzucho on 15/04/19.
//

#ifndef ERROR_CORRECTION_FILEOPERATIONS_H
#define ERROR_CORRECTION_FILEOPERATIONS_H

typedef struct file_part_s {
    size_t bit_amount;
    u_int32_t entire_crc;
    unsigned char *parity_file;
    unsigned char *buffer;
} file_part;

void read_file(char *filename, unsigned char **buffer, size_t *file_length);

void divide_buffer(unsigned char *buffer, file_part **all_parts, int server_amount, size_t file_length);

void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length);

void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file);

void loose_bits(file_part *part_to_loose);

void recover_part(file_part *all_parts, int server_amount, int part_to_recover, unsigned char *parity_file);

void print_descriptive_buffer(file_part * part);

void create_all_servers(int *connection_fds, int server_amount);

void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

bool perform_action(char *buffer, int connection_fd, file_part *part);

#endif //ERROR_CORRECTION_FILEOPERATIONS_H
