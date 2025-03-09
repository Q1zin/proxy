#ifndef LOGGER_H
#define LOGGER_H

int init_logger(char* path, int file_size_limit);

int fini_logger(void);

enum LogLevel {
    LOG_DEBUG = 1,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
};

enum OutputStream {
    STDOUT = 1,
    STDERR,
    FILESTREAM
};

const char* level_names[] = {
    "UNKNOWN",
    "LOG_DEBUG",
    "LOG_INFO",
    "LOG_WARNING",
    "LOG_ERROR",
    "LOG_FATAL"
};

int write_log(enum OutputStream stream, enum LogLevel level, char* filename, int line_number, char* format, ...);

#endif
