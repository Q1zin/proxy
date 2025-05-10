#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <dlfcn.h>
#include <libgen.h>
#include "include/master.h"
#include "include/config.h"
#include "include/logger.h"

Hook executor_start_hook = NULL;

void *plugin_handles[MAX_TOKENS];
int plugin_handles_count = 0;

void load_plugin(const char *plugin) {
    char plugin_path[256];
    snprintf(plugin_path, sizeof(plugin_path), "%s.so", plugin);

    void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Library couldn't be opened.\n \
\tLibrary's path is %s\n \
\tdlopen: %s\n \
\tcheck plugins folder or rename library\n", plugin_path, dlerror());
        return;
    }

    void (*init)(void) = (void (*)(void))dlsym(handle, "init");
    if (!init) {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Library couldn't execute %s.\n \
\tLibrary's name is %s. Dlsym message: %s\n \
\tcheck plugins folder or rename library\n", "init", plugin, dlerror());
        dlclose(handle);
        return;
    }

    plugin_handles[plugin_handles_count++] = handle;
    init();
    write_log(FILESTREAM, LOG_INFO, __FILE__, __LINE__, "Plugin %s loaded", plugin);
}

void unload_plugins() {
    for (int i = plugin_handles_count - 1; i >= 0; --i) {
        void* (*fini)(void) = (void* (*)(void))dlsym(plugin_handles[i], "fini");
        const char* (*name)(void) = (const char* (*)(void))dlsym(plugin_handles[i], "name");
        const char* plugin_name = name ? name() : "unknown";

        if (!fini) {
            write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Library couldn't execute %s.\n \
\tLibrary's name is %s. Dlsym message: %s\n \
\tcheck plugins folder or rename library\n", "fini", plugin_name, dlerror());
        } else {
            fini();
        }

        dlclose(plugin_handles[i]);
    }
    plugin_handles_count = 0;
}


int main(int argc, char *argv[]) {
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        char *filename = basename(path);

        if (strcmp(filename, "debug_proxy") == 0) {
            debug_mode = 1;
        }
    }

    char *config_path = NULL;
    char *logs_path = NULL;

    struct option long_options[] = {
        {"config", required_argument, NULL, 'c'},
        {"logs", required_argument, NULL, 'l'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    opterr = 0;

    while ((opt = getopt_long(argc, argv, "c:l:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_path = optarg;
                break;
            case 'l':
                logs_path = optarg;
                break;
        }
    }

    if (!config_path) {
        config_path = getenv("PROXY_CONFIG_PATH");
    }

    if (!logs_path) {
        logs_path = getenv("PROXY_LOG_PATH");
    }

    if (init_logger(logs_path, -1)) {
        fprintf(stderr, "Failed to initialize the logger\n");
        return 1;
    }

    if (create_config_table()) {
        fprintf(stderr, "Failed to initialize the config\n");
        return 1;
    }

    parse_config(config_path ? config_path : "proxy.conf");

    if (!logs_path && does_variable_exist("logs")) {
        ConfigVariable var = get_variable("logs");
        set_log_file_path(var.data.string[0]);
    }

    if (does_variable_exist("log_file_size_limit")) {
        ConfigVariable var = get_variable("log_file_size_limit");
        set_log_file_size_limit(var.data.integer[0]);
    }

    if (!does_variable_exist("plugins")) {
        char* list_plug_str = getenv("PROXY_MASTER_PLUGINS");
        if (list_plug_str) {
            parse_list(list_plug_str, plugs_list, &plugs_count);
        }
    }

    for (int i = 0; i < plugs_count; i++) {
        load_plugin(plugs_list[i]);
    }
    
    if (executor_start_hook) {
        executor_start_hook();
    }

    unload_plugins();

    if (destroy_config_table()) {
        fprintf(stderr, "Couldn't shut down config\n");
        return 1;
    }

    if (fini_logger()) {
        fprintf(stderr, "Couldn't shut down logger\n");
        return 1;
    }

    return 0;
}
