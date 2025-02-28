#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "include/master.h"
#include "include/config.h"
#include "include/logger.h"

Hook executor_start_hook = NULL;

int main(void) {
    if (create_config_table()) {
        fprintf(stderr, "Failed to initialize the config\n");
        return 1;
    }

    if (init_logger("path", 0)) {
        fprintf(stderr, "Failed to initialize the logger\n");
        return 1;
    }

    char *plug_name = "greeting";
    char *plugin_path = "greeting.so";

    void *handle = dlopen(plugin_path, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Library couldn't be opened.\n \
\tLibrary's path is %s\n \
\tdlopen: %s\n \
\tcheck plugins folder or rename library\n", plugin_path, dlerror());
        return 1;
    }

    char (*name)(void) = (char (*)(void))dlsym(handle, "name");
    if (!name) {
        fprintf(stderr, "Library couldn't execute %s.\n \
\tLibrary's name is %s. Dlsym message: %s\n \
\tcheck plugins folder or rename library\n", "name", plug_name, dlerror());
        dlclose(handle);
        return 1;
    }

    void (*init)(void) = (void (*)(void))dlsym(handle, "init");
    if (!init) {
        fprintf(stderr, "Library couldn't execute %s.\n \
\tLibrary's name is %s. Dlsym message: %s\n \
\tcheck plugins folder or rename library\n", "init", plug_name, dlerror());
        dlclose(handle);
        return 1;
    }

    void (*fini)(void) = (void (*)(void))dlsym(handle, "fini");
    if (!fini) {
        fprintf(stderr, "Library couldn't execute %s.\n \
\tLibrary's name is %s. Dlsym message: %s\n \
\tcheck plugins folder or rename library\n", "fini", plug_name, dlerror());
        dlclose(handle);
        return 1;
    }

    init();

    if (executor_start_hook) {
        executor_start_hook();
    } else {
        fprintf(stderr, "executor_start_hook не подключен!\n");
    }

    fini();

    if (executor_start_hook) {
        fprintf(stderr, "Ошибка: executor_start_hook не отключился!\n");
    } else {
        fprintf(stderr, "executor_start_hook отключен\n");
    }

    if (destroy_config_table()) {
        fprintf(stderr, "Couldn't shut down config\n");
        return 1;
    }

    if (fini_logger()) {
        fprintf(stderr, "Couldn't shut down logger\n");
        return 1;
    }

    dlclose(handle);

    return 0;
}