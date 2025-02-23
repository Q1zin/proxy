#include <stdio.h>
#include "include/logger.h"

static int __init_count_logger = 0;

int init_logger(char* path, int file_size_limit) {
    (void)path;
    (void)file_size_limit;
    if (__init_count_logger == 0) {
        fprintf(stderr, "Logger connected\n");
        __init_count_logger = 1;
        return 0;
    }
    fprintf(stderr, "Logger connected again\n");
    return 1;
}

int fini_logger(void) {
    if (__init_count_logger == 1) {
        fprintf(stderr, "Logger disconnected\n");
        __init_count_logger = 0;
        return 0;
    }
    fprintf(stderr, "Logger disconnected again\n");
    return 1;
}
