#include <stdio.h>
#include <stdlib.h>
#include "master.h"

static Hook previous_hook = NULL;

static void greeting_function(void) {
    if (previous_hook) {
        previous_hook();
    }

    fprintf(stdout, "Hello, world!\n");
}

void init(void) {
    previous_hook = executor_start_hook;
    fprintf(stdout, "greeting initialized\n");
    executor_start_hook = greeting_function;
}

void fini(void) {
    fprintf(stdout, "greeting finished\n");
    executor_start_hook = previous_hook;
}

const char *name(void) {
    return "greeting";
}
