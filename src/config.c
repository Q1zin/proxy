#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include "include/config.h"
#include "include/logger.h"

#define MAX_LINE 256

static int __init_count_config = 0;
ConfigVariable config_variables[MAX_VARIABLES];
int config_count = 0;

char *plugs_list[MAX_TOKENS];
int plugs_count = 0;

static int priorities[MAX_VARIABLES];

int create_config_table(void) {
    if (__init_count_config > 0) {
        fprintf(stderr, "Config connected again\n");
        return 1;
    }
    __init_count_config = 1;
    fprintf(stderr, "Config connected\n");
    return 0;
}

int destroy_config_table(void) {
    if (__init_count_config < 0) {
        fprintf(stderr, "Config disconnected again\n");
        return 1;
    }
    __init_count_config = -1;
    fprintf(stderr, "Config disconnected\n");
    return 0;
}

void trim(char *str) {
    if (!str) return;
    while (isspace((unsigned char)*str)) str++;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

void remove_quotes(char *str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

void free_variable_data(ConfigVariable *var) {
    if (var->type == STRING) {
        for (int i = 0; i < var->count; ++i) {
            free(var->data.string[i]);
        }
        free(var->data.string);
    } else if (var->type == INTEGER) {
        free(var->data.integer);
    } else if (var->type == REAL) {
        free(var->data.real);
    }
    var->type = UNDEFINED;
    var->count = 0;
}

void copy_variable_data(ConfigVariable *dst, const ConfigVariable *src) {
    dst->type = src->type;
    dst->count = src->count;
    switch (src->type) {
        case INTEGER:
            dst->data.integer = malloc(sizeof(int64_t) * src->count);
            memcpy(dst->data.integer, src->data.integer, sizeof(int64_t) * src->count);
            break;
        case REAL:
            dst->data.real = malloc(sizeof(double) * src->count);
            memcpy(dst->data.real, src->data.real, sizeof(double) * src->count);
            break;
        case STRING:
            dst->data.string = malloc(sizeof(char *) * src->count);
            for (int i = 0; i < src->count; i++) {
                dst->data.string[i] = strdup(src->data.string[i]);
            }
            break;
        default:
            break;
    }
}

int define_variable_with_priority(const ConfigVariable variable, int priority) {
    for (int i = 0; i < config_count; ++i) {
        if (strcmp(config_variables[i].name, variable.name) == 0) {
            if (priorities[i] > priority) return 1;
            free_variable_data(&config_variables[i]);
            copy_variable_data(&config_variables[i], &variable);
            priorities[i] = priority;
            return 0;
        }
    }

    if (config_count >= MAX_VARIABLES) return 1;

    ConfigVariable *dst = &config_variables[config_count];
    dst->name = strdup(variable.name);
    dst->description = strdup(variable.description);
    copy_variable_data(dst, &variable);
    priorities[config_count++] = priority;
    return 0;
}

int define_variable(const ConfigVariable variable) {
    return define_variable_with_priority(variable, 0);
}

int set_variable(const ConfigVariable variable) {
    return define_variable_with_priority(variable, 2);
}

ConfigVariable get_variable(const char *name) {
    for (int i = 0; i < config_count; i++) {
        if (strcmp(config_variables[i].name, name) == 0) {
            return config_variables[i];
        }
    }
    return (ConfigVariable){ .type = UNDEFINED };
}

bool does_variable_exist(const char *name) {
    for (int i = 0; i < config_count; i++) {
        if (strcmp(config_variables[i].name, name) == 0) return true;
    }
    return false;
}

void parse_list(char *value, char **list, int *count) {
    *count = 0;
    char *token = strtok(value, "[,]");
    while (token && *count < MAX_TOKENS) {
        trim(token);
        remove_quotes(token);
        list[(*count)++] = strdup(token);
        token = strtok(NULL, "[,]");
    }
}

void process_include_dir(const char *current_path, const char *dir_path, int priority) {
    char resolved_path[PATH_MAX];
    if (realpath(dir_path, resolved_path) == NULL) {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Failed to resolve path: %s - %s\n", dir_path, strerror(errno));
        return;
    }

    DIR *dir = opendir(resolved_path);
    if (!dir) {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Opendir failed: %s. Path: \n", strerror(errno), resolved_path);
        return;
    }
    struct dirent *entry;
    char *filenames[MAX_TOKENS];
    int count = 0;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_REG) continue;
        if (strcmp(entry->d_name, current_path) == 0) continue;
        filenames[count++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(filenames, count, sizeof(char *), (int(*)(const void*, const void*))strcmp);

    for (int i = 0; i < count; ++i) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, filenames[i]);
        parse_config_with_priority(full_path, priority);
        free(filenames[i]);
    }
}

ConfigVariable parse_value_to_variable(const char *key, char *value, const char *path, int line_now, bool *valid) {
    ConfigVariable var = { .name = (char *)key, .description = "" };
    *valid = true;

    if (strchr(value, '.') && strspn(value, "-.0123456789") == strlen(value)) {
        var.type = REAL;
        var.count = 1;
        var.data.real = malloc(sizeof(double));
        *var.data.real = atof(value);
    } else if (strspn(value, "-0123456789") == strlen(value)) {
        var.type = INTEGER;
        var.count = 1;
        var.data.integer = malloc(sizeof(int64_t));
        *var.data.integer = atoll(value);
    } else if (value[0] == '"') {
        remove_quotes(value);
        var.type = STRING;
        var.count = 1;
        var.data.string = malloc(sizeof(char *));
        var.data.string[0] = strdup(value);
    } else if (value[0] == '[') {
        parse_list(value, plugs_list, &plugs_count);
        var.type = STRING;
        var.count = plugs_count;
        var.data.string = malloc(sizeof(char *) * plugs_count);
        for (int i = 0; i < plugs_count; ++i)
            var.data.string[i] = plugs_list[i];
    } else {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "Configuration file %s has syntax error at %d: %s", path, line_now, value);
        *valid = false;
    }
    return var;
}

int parse_config_with_priority(const char *path, int priority) {
    FILE *f = fopen(path, "r");
    if (!f) {
        write_log(FILESTREAM, LOG_ERROR, __FILE__, __LINE__, "File %s not found", path);
        return 1;
    }
    write_log(FILESTREAM, LOG_INFO, __FILE__, __LINE__, "Start read config file %s", path);

    char line[MAX_LINE];
    int line_now = 0;
    while (fgets(line, sizeof(line), f)) {
        line_now++;
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        trim(line);
        if (!*line) continue;

        if (strncmp(line, "include_dir", 11) == 0) {
            char *dir_path = line + 11;
            trim(dir_path);
            process_include_dir(path, dir_path, priority);
            continue;
        }

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        if (!key || !value) continue;
        trim(key);
        trim(value);

        bool valid;
        ConfigVariable var = parse_value_to_variable(key, value, path, line_now, &valid);
        if (!valid) continue;

        define_variable_with_priority(var, priority);
    }
    fclose(f);
    write_log(FILESTREAM, LOG_INFO, __FILE__, __LINE__, "Finish read config file %s", path);
    return 0;
}

int parse_config(const char *path) {
    return parse_config_with_priority(path, 0);
}
