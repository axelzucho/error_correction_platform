#include <stdio.h>
#include <stdlib.h>
#include "libcrc-2.0/include/checksum.h"

#define STR_LEN 100

typedef struct file_part_s{
    u_int32_t entire_crc;
    unsigned char* buffer;
} file_part;

void divide_buffer(unsigned char* buffer, file_part** all_parts, int server_amount){
   *all_parts = malloc(server_amount*sizeof(file_part));
   u_int32_t crc = crc_32(buffer, sizeof(buffer));

   for(int i = 0; i < server_amount; ++i){
       (*all_parts)[i].entire_crc = crc;
       (*all_parts)[i].buffer = malloc(sizeof(buffer)/server_amount + 1);
   }

   for(int i = 0; i < sizeof(buffer); ++i){
       (*all_parts)[i%server_amount].buffer[i/server_amount] = buffer[i];
   }
}

void read_file(char* filename, unsigned char** buffer){
    FILE *file;
    size_t file_length;

    file = fopen(filename, "rb");

    if(file == NULL){
        printf("Can't open file\n");
    }

    fseek(file, 0, SEEK_END);
    file_length = (size_t)ftell(file);
    rewind(file);

    *buffer = malloc((file_length + 1)* sizeof(unsigned char));
    fread(*buffer, file_length, 1, file);

    fclose(file);
}


void menu(){
    char filename[STR_LEN];
    int number_of_servers;
    unsigned char *buffer = NULL;
    file_part* all_parts = NULL;

    printf("Welcome to the error correction testing platform\n");
    printf("Please enter the file you want to test with\n");
    scanf("%s", filename);

    printf("Please enter the number of servers you want to distribute this file to");
    scanf("%d", &number_of_servers);

    read_file(filename, &buffer);
    divide_buffer(buffer, &all_parts, number_of_servers);
    printf("Hello, World!\n");
}

int main() {
    menu();
    return 0;
}