#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    number_of_servers = 3;

    read_file(filename, &buffer, &file_length);

    int *connection_fds = malloc(3 * sizeof(int));
    create_all_servers(connection_fds, 3);
    unsigned char *parity = NULL;
    get_parity(buffer, number_of_servers, file_length, &parity);
    divide_buffer(buffer, parity, &all_parts, number_of_servers, file_length);
    send_all_parts(connection_fds, number_of_servers, all_parts);

    printf("The file was separated and sent to three servers. Each server contains one third of the file\n");
    printf("Please enter the server you want to attack (0, 1, or 2):\n");
    int server_attacked;
    scanf("%d", &server_attacked);

    send_clear_instruction(connection_fds[server_attacked]);
    file_part *new_parts = calloc(sizeof(file_part), (size_t)number_of_servers);
    receive_all_parts(connection_fds, number_of_servers, new_parts);

    memset(buffer, 0, file_length);
    printf("Before recovery, your file looks like this:\n");
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    printf("FILE: %s\n", buffer);
    memset(buffer, 0, file_length);

    printf("After recovery, your file is:\n");
    recover_part(new_parts, number_of_servers, 0, parity);
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    printf("FILE: %s\n", buffer);

    if (all_parts[0].entire_crc == crc_32(buffer, file_length)) printf("File successfully recovered... They are the same!\n");
}

int main() {
    menu();
    return 0;
}