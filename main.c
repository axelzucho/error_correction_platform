#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcrc-2.0/include/checksum.h"

#include "FileOperations.h"
#include "sockets/FileTransmission.h"
#include "tools.h"

#define STR_LEN 100000

void cat_before_ext(char *filename, char* adding, char dest[STR_LEN]){
    char *raw = strchr(filename, '.');
    strncpy(dest, filename, raw - filename);
    strcat(dest, adding);
    strcat(dest, raw);
}


void menu() {
    char filename[STR_LEN];
    int number_of_servers;
    unsigned char *buffer = NULL;
    file_part *all_parts = NULL;
    size_t file_length;;

    printf("Welcome to the error correction testing platform\n");
    printf("Please enter the file you want to test with:\n");

    //scanf("%s", filename);
    strcpy(filename, "HOT_Balloon_Trip.ppm");

    number_of_servers = 3;

    read_file(filename, &buffer, &file_length);

    int *connection_fds = malloc(number_of_servers * sizeof(int));
    create_all_servers(connection_fds, 3);
    unsigned char *parity = NULL;
    get_parity(buffer, number_of_servers, file_length, &parity);
    divide_buffer(buffer, parity, &all_parts, number_of_servers, file_length);
    printf("Before sending all parts\n");
    send_all_parts(connection_fds, number_of_servers, all_parts);
    printf("After sending all parts\n");
    u_int32_t entire_crc = crc_32(buffer, file_length);

    printf("The file was separated and sent to three servers. Each server contains one third of the file\n");
    printf("Please enter the server you want to attack (0, 1, or 2):\n");
    int server_attacked;
    //scanf("%d", &server_attacked);
    server_attacked = 0;
    free_parts(&all_parts, number_of_servers);

    send_clear_instruction(connection_fds[server_attacked]);
    file_part *new_parts = calloc(sizeof(file_part), (size_t)number_of_servers);
    receive_all_parts(connection_fds, number_of_servers, new_parts);

    memset(buffer, 0, file_length);
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    char broken_file[STR_LEN];
    cat_before_ext(filename, "_broken", broken_file);
    write_file(broken_file, buffer, file_length);
    printf("File before recovery written\n");

    memset(buffer, 0, file_length);

    recover_part(new_parts, number_of_servers, server_attacked, parity);
    merge_parts(new_parts, number_of_servers, buffer, file_length);
    printf("After recovery, your file is:\n");
    //printf("FILE: %s\n", buffer);

    if (entire_crc == crc_32(buffer, file_length)) {
        printf("File successfully recovered... They are the same!\n");
    } else {
        printf("Hmmm, this wasn't recovered correctly... Tough one!\n");
    }

    char recovered_file[STR_LEN];
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