#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libcrc-2.0/include/checksum.h"

#define STR_LEN 100

typedef struct file_part_s{
    size_t bit_amount;
    u_int32_t entire_crc;
    unsigned char* buffer;
} file_part;

void divide_buffer(unsigned char* buffer, file_part** all_parts, int server_amount,size_t file_length){
   *all_parts = malloc(server_amount*sizeof(file_part));
   u_int32_t crc = crc_32(buffer, file_length);

   for(int i = 0; i < server_amount; ++i){
       (*all_parts)[i].entire_crc = crc;
       (*all_parts)[i].buffer = calloc(file_length/(server_amount*8) + 1, sizeof(unsigned char));
       (*all_parts)[i].bit_amount = (file_length*8)/server_amount + 1;
   }

   for(int i = 0; i < file_length * 8; ++i){
       bool current_val = (bool)(buffer[i/8] & (1 << (7 - i % 8)));
       (*all_parts)[i%server_amount].buffer[i/(server_amount*8)] |= (current_val << (7 - (i / server_amount) % 8));
   }
}

void merge_parts(file_part * all_parts, int server_amount, unsigned char* buffer, size_t file_length){
    for(int i = 0; i< file_length * 8; ++i){
        bool current_val = (bool)(all_parts[i%server_amount].buffer[i/(server_amount*8)] & (1 << (7 - (i/server_amount) % 8)));
        buffer[i/8] |= current_val << (7 - (i % 8));
    }
}

void read_file(char* filename, unsigned char** buffer, size_t *file_length){
    FILE *file;

    file = fopen(filename, "rb");

    if(file == NULL){
        printf("Can't open file\n");
    }

    fseek(file, 0, SEEK_END);
    *file_length = (size_t)ftell(file);
    rewind(file);

    *buffer = malloc((*file_length + 1)* sizeof(unsigned char));
    fread(*buffer, *file_length, 1, file);

    fclose(file);
}


void menu(){
    printf("%d", 1 << 7);
    char filename[STR_LEN];
    int number_of_servers;
    unsigned char *buffer = NULL;
    file_part* all_parts = NULL;
    size_t file_length;
;
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

    if(all_parts[0].entire_crc == crc_32(buffer, file_length)) printf("They are the same!\n");

    printf("FILE: %s\n", buffer);
}

int main() {
    menu();
    return 0;
}