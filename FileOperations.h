// Axel Zuchovicki A01022875

#ifndef ERROR_CORRECTION_FILEOPERATIONS_H
#define ERROR_CORRECTION_FILEOPERATIONS_H

#include <stdlib.h>

typedef struct file_part_s {
    unsigned long bit_amount;
    unsigned char *buffer;
    unsigned long parity_size;
    unsigned char *parity_file;
} file_part;

#define FILE_OPEN_ERROR -1

// Reads a file into the buffer and return its size.
int read_file(char *filename, unsigned char **buffer, size_t *file_length);

// Writes a buffer into the specified file for the 'file_length' amount of bits.
int write_file(char *filename, unsigned char *buffer, size_t file_length);

// Divides a buffer into 'server_amount' of parts, bit by bit.
void divide_buffer(unsigned char *buffer, unsigned char *parity, file_part **all_parts, int server_amount,
                   size_t file_length);

// Merges the bits from the parts into a buffer.
void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length);

// Gets the parity file for the sent buffer and saves it into 'parity_file'.
void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file);

// Simulates the loss of bits for the sent part.
void loose_bits(file_part *part_to_loose);

// Recovers the lost part by using the parity file to set the unset bits.
void recover_part(file_part *all_parts, int server_amount, int part_to_recover, unsigned char *parity_file);

// Prints the part for debugging.
void print_descriptive_buffer(file_part *part);

// Frees all the memory allocated by the 'server_amount' of parts.
void free_parts(file_part **parts, int server_amount);

// Handles the reading error; outputs information to the terminal.
void handle_reading_error(int error, char *filename);

#endif //ERROR_CORRECTION_FILEOPERATIONS_H
