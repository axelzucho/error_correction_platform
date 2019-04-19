#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libcrc-2.0/include/checksum.h"

#include "FileOperations.h"
#include "sockets/FileTransmission.h"

#define STR_LEN 100

void menu() {
    char filename[STR_LEN];
    int number_of_servers;
    unsigned char *buffer = NULL;
    file_part *all_parts = NULL;
    size_t file_length;;
    printf("Welcome to the error correction testing platform\n");
    printf("Please enter the file you want to test with:\n");
    //scanf("%s", filename);
    strcpy(filename, "../example.txt");

    printf("Please enter the number of servers you want to distribute this file to:\n");
    //scanf("%d", &number_of_servers);
    number_of_servers = 3;

    read_file(filename, &buffer, &file_length);

    int *connection_fds = malloc(3 * sizeof(int));
    create_all_servers(connection_fds, 3);
    unsigned char *parity = NULL;

    get_parity(buffer, number_of_servers, file_length, &parity);
    divide_buffer(buffer, &all_parts, number_of_servers, file_length);
    send_all_parts(connection_fds, number_of_servers, all_parts);
    file_part * new_parts = calloc(sizeof(file_part), number_of_servers);
    receive_all_parts(connection_fds, number_of_servers, new_parts);
    memset(buffer, 0, file_length);
    merge_parts(all_parts, number_of_servers, buffer, file_length);
    loose_bits(&all_parts[1]);
    recover_part(all_parts, number_of_servers, 1, parity);
    merge_parts(all_parts, number_of_servers, buffer, file_length);

    if (all_parts[0].entire_crc == crc_32(buffer, file_length)) printf("They are the same!\n");

    printf("FILE: %s\n", buffer);
}

int main() {
    menu();
    return 0;
}