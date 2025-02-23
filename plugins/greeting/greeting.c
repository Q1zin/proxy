#include <stdio.h>
#include <stdlib.h>
#include "master.h"

static void greeting_function(void) {
    fprintf(stdout, "Hello, world!\n");
}

void init(void) {
    fprintf(stdout, "greeting initialized\n");
    executor_start_hook = greeting_function;
}

void fini(void) {
    fprintf(stdout, "greeting finished\n");
    executor_start_hook = NULL;
}

const char *name(void) {
    return "greeting";
}
