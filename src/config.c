#include <stdio.h>
#include "include/config.h"

static int __init_count_config = 0;

int create_config_table(void) {
    if (__init_count_config == 0) {
        fprintf(stderr, "Config connected\n");
        __init_count_config = 1;
        return 0;
    }
    fprintf(stderr, "Config connected again\n");
    return 1;
}

int destroy_config_table(void) {
    if (__init_count_config == 1) {
        fprintf(stderr, "Config disconnected\n");
        __init_count_config = 0;
        return 0;
    }
    fprintf(stderr, "Config disconnected again\n");
    return 1;
}
