// Axel Zuchovicki A01022875

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "FileOperations.h"

void divide_buffer(unsigned char *buffer, unsigned char *parity, file_part **all_parts, int server_amount,
                   size_t file_length) {
    *all_parts = malloc(server_amount * sizeof(file_part));
    // The size for the parity file is the file length divided by the amount of servers. We must apply the ceiling
    // to make sure we catch all the information and not loose any bits.
    size_t parity_size = (size_t) ceil((double) file_length / server_amount);
    // Initialize all parts.
    for (int i = 0; i < server_amount; ++i) {
        (*all_parts)[i].buffer = calloc((size_t) ceil((double) file_length / server_amount), sizeof(unsigned char));
        // The amounts of bits is defined by the bytes times 8, divided by the server amount.
        // We need to check if the specific part has an extra bit, that is the last addition.
        (*all_parts)[i].bit_amount = (file_length * 8) / server_amount + (i < file_length % server_amount);

        // If it is one of the first to servers, then we need to copy the parity file.
        if (i < 2) {
            (*all_parts)[i].parity_size = parity_size;
            (*all_parts)[i].parity_file = malloc(parity_size * sizeof(unsigned char));
            strncpy((char *) (*all_parts)[i].parity_file, (char *) parity, parity_size);
            // If it isn't then we just set the size to 0 and don't initialize it.
        } else {
            (*all_parts)[i].parity_size = 0;
            (*all_parts)[i].parity_file = NULL;
        }
    }

#pragma omp parallel for default(none) shared(all_parts, server_amount, file_length, buffer)
    // Iterates for each bit.
    for (unsigned long i = 0; i < file_length * 8; ++i) {
        // If it is less than the amount of bits for the part, then just continue.
        if (i / server_amount > (*all_parts)[i % server_amount].bit_amount) {
            continue;
        }
        // Get the value for the bit currently setting.
        bool current_val = (bool) (buffer[i / 8] & (1 << (7 - i % 8)));
        // Get the amount of left shift for the divided buffer.
        int shift_amount = 7 - (int) ((i / server_amount) % 8);

        // Set the bit in the correct server.
        (*all_parts)[i % server_amount].buffer[i / (server_amount * 8)] |= current_val << shift_amount;
    }
}

void merge_parts(file_part *all_parts, int server_amount, unsigned char *buffer, size_t file_length) {
#pragma omp parallel for default(none) shared(all_parts, server_amount, file_length, buffer)
    // Iterates through all the bits.
    for (unsigned long i = 0; i < file_length * 8; ++i) {
        // If it is less than the amount of bits for the part, then just continue.
        if (i / server_amount >= all_parts[i % server_amount].bit_amount) {
            continue;
        }
        // The current value is set by the bit from the part that is its turn, left-shifted by
        // the position for the bit in that part. Similar to the procedure in the buffer division.
        bool current_val = (bool) (all_parts[i % server_amount].buffer[i / (server_amount * 8)] &
                                   (1 << (7 - (i / server_amount) % 8)));
        // The left-shift for the bit that will be set in the buffer is calculated.
        int shift_amount = 7 - (int) (i % 8);

        // The bit in the buffer is set.
        buffer[i / 8] |= current_val << shift_amount;
    }
}

int read_file(char *filename, unsigned char **buffer, size_t *file_length) {
    FILE *file;

    file = fopen(filename, "rb");

    if (file == NULL) {
        return FILE_OPEN_ERROR;
    }

    // Get the file size by going to the end of the file.
    fseek(file, 0, SEEK_END);
    *file_length = (size_t) ftell(file);
    // Go back to the beginning.
    rewind(file);

    // Initialize the buffer
    *buffer = malloc((*file_length) * sizeof(unsigned char));
    // Read the bits into the buffer.
    fread(*buffer, *file_length, 1, file);

    fclose(file);
    return 0;
}

void get_parity(unsigned char *buffer, int server_amount, size_t file_length, unsigned char **parity_file) {
    // Initialize the parity file.
    *parity_file = calloc((size_t) ceil((double) file_length / server_amount), sizeof(unsigned char));

#pragma omp parallel for default(none) shared(buffer, server_amount, file_length, parity_file)
    // Iterate through all the bits.
    for (unsigned long i = 0; i < (file_length * 8); i += server_amount) {
        // Start the bit in false.
        bool current_value = false;
        // Iterate through all the servers.
        for (int j = 0; j < server_amount; j++) {
            // If the bit corresponding to that server is set, then change the parity value.
            if (buffer[(i + j) / 8] & (1 << (7 - (i + j) % 8))) {
                current_value = !current_value;
            }
        }
        // Calculate the shift for the bit in the parity file.
        int shift_value = 7 - (int) ((i / server_amount) % 8);
        // Set the bit.
        (*parity_file)[i / (8 * server_amount)] |= current_value << shift_value;
    }

    bool current_value = false;
    // Iterate through the last bits that might not have completed the loop, not setting the value.
    for (unsigned long i = file_length * 8 - file_length % server_amount; i < file_length * 8; i++) {
        if (buffer[i / 8] & (1 << (7 - i % 8))) {
            current_value = !current_value;
        }
    }

    // If we need to set the final bit for the parity file.
    if (file_length % server_amount != server_amount - 1) {
        // Set the bit the same way that we did in the loop.
        int final_shift = 7 - (int) (file_length * 8) / server_amount % 8;
        (*parity_file)[file_length / server_amount] |= current_value << final_shift;
    }
}

void loose_bits(file_part *part_to_loose) {
    // Set all the bits to 0.
    memset(part_to_loose->buffer, 0, (size_t) ceil((double) part_to_loose->bit_amount / 8));
    // If it has a parity file.
    if (part_to_loose->parity_size > 0) {
        // Set all the bits in the parity file ot 0.
        memset(part_to_loose->parity_file, 0, (size_t) part_to_loose->parity_size);
        part_to_loose->parity_size = 0;
    }
    part_to_loose->bit_amount = 0;
}

void recover_part(file_part *all_parts, int server_amount, int part_to_recover, unsigned char *parity_file) {
    // If we lost the first part, set the reference for the parity file to the second.
    int reference = part_to_recover == 0 ? 1 : 0;
    // An extra bit would ensure that we never loose information. If that extra bit is a 0, then it doesn't affect the file.
    all_parts[part_to_recover].bit_amount = all_parts[reference].bit_amount + 1;
    // Re-init the buffer for the lost part.
    all_parts[part_to_recover].buffer = calloc((size_t) ceil((double) all_parts[part_to_recover].bit_amount / 8),
                                               sizeof(unsigned char));

    // Iterate till an extra bit for the special case.
#pragma omp parallel for default(none) shared(parity_file, all_parts, server_amount, part_to_recover, reference)
    for (unsigned long i = 0; i <= all_parts[reference].bit_amount; i++) {
        // Start with an unset bit.
        bool current_value = false;
        for (int j = 0; j < server_amount; j++) {
            if (all_parts[j].bit_amount <= i || j == part_to_recover) continue;
            // Keep track of the set bits in current value.
            if (all_parts[j].buffer[i / 8] & (1 << (7 - i % 8))) {
                current_value = !current_value;
            }
        }
        // Get the corresponding bit in the parity file.
        bool current_parity = (bool) (parity_file[i / 8] & 1 << (7 - i % 8));
        // If the bit calculated is different than the one in the parity file
        if (current_parity != current_value && all_parts[part_to_recover].bit_amount > i) {
            // We need to set the bit in the part that was lost.
            all_parts[part_to_recover].buffer[i / 8] |= 1 << (7 - i % 8);
        }
    }

}

void print_descriptive_buffer(file_part *part) {
    for (unsigned long i = 0; i < part->bit_amount / 8; ++i) {
        // Cast the byte to an int to see easily its value.
        printf("%d ", (int) (part->buffer[i]));
    }
    printf("\n");
}

// Frees the memory allocated by the sent part.
void free_part(file_part *part) {
    if (part->buffer != NULL) {
        free(part->buffer);
    }
    // Maybe the parity file was not set.
    if (part->parity_file != NULL) {
        free(part->parity_file);
    }
}

void free_parts(file_part **parts, int server_amount) {
    // Free all the parts before freeing the array.
    for (int i = 0; i < server_amount; ++i) {
        free_part(&((*parts)[i]));
    }
    free(*parts);
}

int write_file(char *filename, unsigned char *buffer, size_t file_length) {
    FILE *file;

    file = fopen(filename, "wb");

    if (file == NULL) {
        return FILE_OPEN_ERROR;
    }

    fwrite(buffer, file_length, sizeof(unsigned char), file);

    fclose(file);
    return 0;
}

void handle_reading_error(int error, char *filename) {
    if (error == FILE_OPEN_ERROR) {
        printf("Can't open file %s\n", filename);
    } else {
        printf("Unknown reading error for file %s\n", filename);
    }
}
