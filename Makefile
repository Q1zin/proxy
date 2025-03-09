PROXY_BASE_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CC ?= gcc
CCFLAGS ?=
CFLAGS ?= -Wall -Wextra -Wpedantic -Wpointer-arith -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough -Wcast-function-type -Wshadow -Wformat-security $(CCFLAGS)
COPT ?=
CPPFLAGS ?= -I./src/include
LDFLAGS ?= -rdynamic -ldl $(CCFLAGS) -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN'

BUILD_DIR = install
PLUGINS_DIR = $(BUILD_DIR)/plugins
GREETING_PLUGIN_DIR = $(PLUGINS_DIR)

SRC_FILES = src/config.c src/master.c
OBJ_FILES = $(addprefix $(BUILD_DIR)/, $(notdir $(SRC_FILES:.c=.o)))

STATIC_LIB = $(BUILD_DIR)/libconfig.a

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    AR_CMD = libtool -static -o
else
    AR_CMD = ar rcs
endif

all: $(BUILD_DIR)/liblogger.so $(BUILD_DIR)/proxy $(GREETING_PLUGIN_DIR)/greeting.so $(STATIC_LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PLUGINS_DIR): | $(BUILD_DIR)
	mkdir -p $(PLUGINS_DIR)

$(GREETING_PLUGIN_DIR): | $(PLUGINS_DIR)
	mkdir -p $(GREETING_PLUGIN_DIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COPT) $(CPPFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/proxy: $(OBJ_FILES) $(STATIC_LIB) $(PROXY_BASE_DIR)/$(BUILD_DIR)/liblogger.so
# $(BUILD_DIR)/proxy: $(OBJ_FILES) $(STATIC_LIB) $(BUILD_DIR)/liblogger.so
	$(CC) $(CFLAGS) $(COPT) $(CPPFLAGS) $(LDFLAGS) $^ -o $@ -llogger

$(STATIC_LIB): $(BUILD_DIR)/config.o $(BUILD_DIR)/master.o
	$(AR_CMD) $@ $^

$(GREETING_PLUGIN_DIR)/greeting.so: plugins/greeting/greeting.c | $(GREETING_PLUGIN_DIR)
	$(CC) $(CFLAGS) $(COPT) $(CPPFLAGS) -fPIC -shared -Wl,-undefined,dynamic_lookup $< -o $@

$(BUILD_DIR)/liblogger.so: src/logger.c src/my_time.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COPT) $(CPPFLAGS) -fPIC -shared $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean