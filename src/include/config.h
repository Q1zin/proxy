#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_VARIABLES 100
#define MAX_TOKENS 100

union ConfigData {
    int64_t* integer;
    double* real;
    char** string;
};

enum ConfigVarType {
    UNDEFINED = 0,
    INTEGER = 1,
    REAL,
    STRING,
};

struct ConfigVariable {
    char* name;
    char* description;
    union ConfigData data;
    enum ConfigVarType type;
    int count;
};

typedef struct ConfigVariable ConfigVariable;

extern char *plugs_list[MAX_TOKENS];
extern int plugs_count;

extern struct ConfigVariable config_variables[MAX_VARIABLES];
extern int config_count;

int create_config_table(void);

int destroy_config_table(void);

int parse_config(const char* path);

int define_variable(const struct ConfigVariable variable);

struct ConfigVariable get_variable(const char* name);

int set_variable(const struct ConfigVariable variable);

bool does_variable_exist(const char* name);

void parse_list(char *value, char **list, int *count);

#endif
