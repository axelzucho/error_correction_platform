#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libcrc-2.0/include/checksum.h"

#include "FileOperations.h"

#define STR_LEN 100


void menu() {
    printf("%d", 1 << 7);
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
    number_of_servers = 10;

    read_file(filename, &buffer, &file_length);
    divide_buffer(buffer, &all_parts, number_of_servers, file_length);
    memset(buffer, 0, file_length);
    merge_parts(all_parts, number_of_servers, buffer, file_length);

    if (all_parts[0].entire_crc == crc_32(buffer, file_length)) printf("They are the same!\n");

    printf("FILE: %s\n", buffer);
}

int main() {
    menu();
    return 0;
}