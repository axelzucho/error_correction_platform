//
// Created by axelzucho on 19/04/19.
//

#ifndef ERROR_CORRECTION_FILETRANSMISSION_H
#define ERROR_CORRECTION_FILETRANSMISSION_H

#include <stdbool.h>

#include "../FileOperations.h"

void create_all_servers(int *connection_fds, int server_amount);

void send_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

void receive_all_parts(int *connection_fds, int server_amount, file_part *all_parts);

bool perform_action(char *buffer, int connection_fd, file_part *part);

#endif //ERROR_CORRECTION_FILETRANSMISSION_H
