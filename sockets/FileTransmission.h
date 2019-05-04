//
// Created by axelzucho on 19/04/19.
//

#ifndef ERROR_CORRECTION_FILETRANSMISSION_H
#define ERROR_CORRECTION_FILETRANSMISSION_H

#include <stdbool.h>

#include "../FileOperations.h"

// Creates all the servers to hold the file parts.
void create_all_servers(int *connection_fds, int server_amount);

// Sends all the parts, one for each server.
void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

// Receives all the parts from each of the servers.
void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

// Sends the clear instruction to the specified server.
void send_clear_instruction(int connection_fd);

#endif //ERROR_CORRECTION_FILETRANSMISSION_H
