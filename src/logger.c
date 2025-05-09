#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "include/logger.h"
#include "include/my_time.h"

int debug_mode = 0;

static int __init_count_logger = 0;
char* __log_file_path = NULL;
int __file_size_limit = -1;

const char* level_names[] = {
    "UNKNOWN",
    "LOG_DEBUG",
    "LOG_INFO",
    "LOG_WARNING",
    "LOG_ERROR",
    "LOG_FATAL"
};

int init_logger(char* path, int file_size_limit) {
    __log_file_path = path;
    __file_size_limit = file_size_limit;
    
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

void set_log_file_path(char* path) {
    __log_file_path = path;
}

void set_log_file_size_limit(int file_size_limit) {
    __file_size_limit = file_size_limit;
}

char* get_utc(char* buffer, size_t buffer_size) {
    time_t time = get_time();
    struct tm* time_info = gmtime(&time);
    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S(UTC)", time_info);
    return buffer;
}

void log_to_stream(FILE* stream, const char* time_buffer, const char* filename, int line_number, int pid, const char* level_str, const char* format, va_list args) {
    fprintf(stream, "%s %s %d [%d] | %s: ", time_buffer, filename, line_number, pid, level_str);
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
}

void log_to_file(const char* time_buffer, const char* filename, int line_number, int pid, const char* level_str, const char* format, va_list args) {
    int fd = open(__log_file_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_to_stream(stderr, time_buffer, filename, line_number, pid, level_str, format, args);
        return;
    }

    struct stat st;
    fstat(fd, &st);
    off_t size_kb = (st.st_size + 1023) / 1024;

    char buffer[1024];
    int msg_size = vsnprintf(buffer, sizeof(buffer), format, args);

    if (__file_size_limit != -1 && size_kb + (msg_size / 1024) >= __file_size_limit) {
        if (ftruncate(fd, 0) != -1) {
            lseek(fd, 0, SEEK_SET);
        }
    }

    dprintf(fd, "%s %s %d [%d] | %s: %s\n", time_buffer, filename, line_number, pid, level_str, buffer);
    close(fd);
}

// "TIME(UTC) FILE LINE [PID] | LEVEL: MESSAGE"
int write_log(enum OutputStream stream, enum LogLevel level, char* filename, int line_number, char* format, ...) {
    const char* level_str = (level >= LOG_DEBUG && level <= LOG_FATAL) ? level_names[level] : "UNKNOWN";

    char time_buffer[32];
    get_utc(time_buffer, sizeof(time_buffer));

    int pid = getpid();

    va_list args;
    va_start(args, format);

    if (debug_mode || stream == STDOUT) {
        log_to_stream(stdout, time_buffer, filename, line_number, pid, level_str, format, args);
    } else if (stream == FILESTREAM && __log_file_path != NULL) {
        log_to_file(time_buffer, filename, line_number, pid, level_str, format, args);
    } else {
        log_to_stream(stderr, time_buffer, filename, line_number, pid, level_str, format, args);
    }
    
    va_end(args);
    return 0;
}
