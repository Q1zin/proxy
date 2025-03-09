#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "include/logger.h"
#include "include/my_time.h"

static int __init_count_logger = 0;
char* __log_file_path = NULL;
int __file_size_limit = -1;

struct stat st;

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

char* get_utc(char* buffer, size_t buffer_size) {
    time_t time = get_time();
    struct tm* time_info = gmtime(&time);
    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S(UTC)", time_info);
    return buffer;
}

// "TIME(UTC) FILE LINE [PID] | LEVEL: MESSAGE"
int write_log(enum OutputStream stream, enum LogLevel level, char* filename, int line_number, char* format, ...) {
    const char* level_str = (level >= LOG_DEBUG && level <= LOG_FATAL) ? level_names[level] : "UNKNOWN";

    char time_buffer[32];
    get_utc(time_buffer, sizeof(time_buffer));

    int pid = getpid();

    va_list args;
    va_start(args, format);

    if (stream == STDOUT) {
        printf("%s %s %d [%d] | %s: ", time_buffer, filename, line_number, pid, level_str);
        vprintf(format, args);
        printf("\n");
    } else if (stream == FILESTREAM && __log_file_path != NULL) {
        int fd = open(__log_file_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            fprintf(stderr, "%s %s %d [%d] | %s: ", time_buffer, filename, line_number, pid, level_str);
            vfprintf(stderr, format, args);
            fprintf(stderr, "\n");
            va_end(args);
            return 0;
        }

        fstat(fd, &st);
        off_t size_kb = (st.st_size + 1023) / 1024;

        char buffer[1024];
        int msg_size = vsnprintf(buffer, sizeof(buffer), format, args);

        if (__file_size_limit != -1 && size_kb + (msg_size / 1024) >= __file_size_limit) {
            ftruncate(fd, 0);
            lseek(fd, 0, SEEK_SET);
        }

        dprintf(fd, "%s %s %d [%d] | %s: ", time_buffer, filename, line_number, pid, level_str);
        dprintf(fd, "%s\n", buffer);

        close(fd);
    } else {
        fprintf(stderr, "%s %s %d [%d] | %s: ", time_buffer, filename, line_number, pid, level_str);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    
    va_end(args);
    return 0;
}