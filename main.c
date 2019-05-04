#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcrc-2.0/include/checksum.h"

#include "FileOperations.h"
#include "sockets/FileTransmission.h"
#include "tools.h"


void cat_before_ext(char *filename, char* adding, char* dest){
    char *raw = strchr(filename, '.');
    char *next_raw = strchr(raw+1, '.');

    while (next_raw != NULL){
        raw = next_raw;
        next_raw = strchr(next_raw+1, '.');
    }

    strncpy(dest, filename, raw - filename);
    strcat(dest, adding);
    strcat(dest, raw);
}


void menu() {
    char filename[FILENAME_MAX];
    int number_of_servers;
    unsigned char *buffer = NULL;
    file_part *all_parts = NULL;
    size_t file_length;;

    printf("Welcome to the error correction testing platform\n");
    printf("Please enter the file you want to test with:\n");

    //scanf("%s", filename);
    strcpy(filename, "example.1.1.2.txt");

    number_of_servers = 3;

    read_file(filename, &buffer, &file_length);

    int *connection_fds = malloc(number_of_servers * sizeof(int));
    create_all_servers(connection_fds, 3);
    unsigned char *parity = NULL;
    get_parity(buffer, number_of_servers, file_length, &parity);
    divide_buffer(buffer, parity, &all_parts, number_of_servers, file_length);
    send_all_parts(connection_fds, number_of_servers, all_parts);
    free_parts(&all_parts, number_of_servers);
    u_int32_t entire_crc = crc_32(buffer, file_length);

    printf("The file was separated and sent to three servers. Each server contains one third of the file\n");
    printf("Please enter the server you want to attack (0, 1, or 2):\n");
    int server_attacked;
    //scanf("%d", &server_attacked);
    server_attacked = 0;

    send_clear_instruction(connection_fds[server_attacked]);
    file_part *new_parts = calloc(sizeof(file_part), (size_t)number_of_servers);
    receive_all_parts(connection_fds, number_of_servers, new_parts);

    memset(buffer, 0, file_length);
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    char broken_file[FILENAME_MAX];
    cat_before_ext(filename, "_broken", broken_file);
    write_file(broken_file, buffer, file_length);
    printf("File before recovery written\n");

    memset(buffer, 0, file_length);

    recover_part(new_parts, number_of_servers, server_attacked, parity);
    merge_parts(new_parts, number_of_servers, buffer, file_length);

    if (entire_crc == crc_32(buffer, file_length)) {
        printf("File successfully recovered... They are the same!\n");
    } else {
        printf("Hmmm, this wasn't recovered correctly... Tough one!\n");
    }

    char recovered_file[FILENAME_MAX];
    cat_before_ext(filename, "_recovered", recovered_file);
    write_file(recovered_file, buffer, file_length);
    printf("File after recovery written\n");

    free(connection_fds);
    free_parts(&new_parts, number_of_servers);
    free(buffer);
}

int main() {
    menu();
    return 0;
}