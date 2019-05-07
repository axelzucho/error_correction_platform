// Axel Zuchovicki A01022875

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "FileOperations.h"
#include "sockets/FileTransmission.h"


// Function to add a string before the extension.
void cat_before_ext(char *filename, char *adding, char *dest) {
    char *raw = strchr(filename, '.');
    char *next_raw = strchr(raw + 1, '.');

    // While there are still '.' to be found.
    while (next_raw != NULL) {
        raw = next_raw;
        next_raw = strchr(next_raw + 1, '.');
    }

    // First copy the filename without extension.
    strncpy(dest, filename, raw - filename);
    // Then the adding string.
    strcat(dest, adding);
    // Finally the extension.
    strcat(dest, raw);
}


void menu() {
    char filename[FILENAME_MAX];
    int number_of_servers;
    unsigned char *buffer = NULL;
    unsigned char *initial_file = NULL;
    file_part *all_parts = NULL;
    size_t file_length;;

    printf("Welcome to the error correction testing platform\n");
    printf("Please enter the file you want to test with:\n");

    scanf("%s", filename);
    //strcpy(filename, "mosaic_090.tif");

    // Number of servers already defined, since this implementation makes more sense this way.
    number_of_servers = 3;

    // Read the bits of the file into the string.
    int read_result = read_file(filename, &initial_file, &file_length);
    if (read_result < 0) {
        handle_reading_error(read_result, filename);
        return;
    }

    // Create the fd array for the servers, and create the servers.
    int *connection_fds = malloc(number_of_servers * sizeof(int));
    create_all_servers(connection_fds, 3);
    unsigned char *parity = NULL;
    // Gets the parity for the file and the specific number of servers.
    get_parity(initial_file, number_of_servers, file_length, &parity);
    // Divides the file into the struct all_parts.
    divide_buffer(initial_file, parity, &all_parts, number_of_servers, file_length);
    // Sends each part to a server.
    send_all_parts(connection_fds, number_of_servers, all_parts);
    // Frees the parts already sent.
    free_parts(&all_parts, number_of_servers);

    printf("The file was separated and sent to three servers. Each server contains one third of the file\n");
    printf("Please enter the server you want to attack (0, 1, or 2):\n");
    int server_attacked;
    //scanf("%d", &server_attacked);
    server_attacked = 0;

    // Send the clear instruction to the server chosen.
    send_clear_instruction(connection_fds[server_attacked]);
    // Create the array of new parts.
    file_part *new_parts = calloc(sizeof(file_part), (size_t) number_of_servers);
    // Receive all parts from each server.
    receive_all_parts(connection_fds, number_of_servers, new_parts);

    // Allocate a new buffer where the file will be restored.
    buffer = calloc(file_length, sizeof(unsigned char));
    // Merges the part into the buffer.
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    char broken_file[FILENAME_MAX];
    memset(broken_file, 0, FILENAME_MAX);
    cat_before_ext(filename, "_broken", broken_file);
    // Writes the file without recovery.
    write_file(broken_file, buffer, file_length);
    printf("File before recovery written\n");

    memset(buffer, 0, file_length);
    // Recovers the part missing.
    recover_part(new_parts, number_of_servers, server_attacked, parity);
    // Merges the parts into the buffer.
    merge_parts(new_parts, number_of_servers, buffer, file_length);

    char recovered_file[FILENAME_MAX];
    memset(recovered_file, 0, FILENAME_MAX);
    cat_before_ext(filename, "_recovered", recovered_file);
    // Writes the recovered file.
    write_file(recovered_file, buffer, file_length);
    printf("File after recovery written\n");

    // Check if both files match.
    if (strncmp((char *) buffer, (char *) initial_file, file_length) == 0) {
        printf("File successfully recovered... They are the same!\n");
    } else {
        printf("Hmmm, this wasn't recovered correctly... Tough one!\n");
    }

    for (int i = 0; i < number_of_servers; i++) {
        wait(NULL);
    }

    // Frees all the allocated memory.
    free(parity);
    free(connection_fds);
    free_parts(&new_parts, number_of_servers);
    free(buffer);
    free(initial_file);
}

int main() {
    menu();
    return 0;
}