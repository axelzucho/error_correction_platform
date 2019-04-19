// Axel Zuchovicki A01022875

#include "fatal_error.h"

void fatalError(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
