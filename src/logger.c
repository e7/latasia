#include <stdarg.h>
#include "file.h"
#include "logger.h"


lts_logger_t lts_stderr_logger = {
    &lts_stderr_file,
};
lts_logger_t lts_file_logger = {
    &lts_log_file,
};


ssize_t lts_write_logger_fd(lts_logger_t *log, void const *buf, size_t n)
{
    return lts_file_write(log->file, buf, n, 0);
}


ssize_t lts_write_logger(lts_logger_t *log,
                         int level, char const *fmt, ...)
{
    // todo: 该函数io次数过多
    va_list args;
    char const *p, *last;
    char const *arg;
    ssize_t total_size, tmp_size;

    va_start(args, fmt);
    total_size = 0;
    p = fmt;
    while (*p) {
        for (last = p; (*last) && ('%' != *last); ++last) {
        }
        tmp_size = lts_write_logger_fd(log, p, last - p);
        if ((0 == *last) ||  (0 == *(last + 1))) {
            break;
        }
        switch (*++last) {
            case 's': {
                arg = va_arg(args, char const *);
                (void)lts_write_logger_fd(log, arg, strlen(arg));
                p = last + 1;
                break;
            }

            case '%': {
                (void)lts_write_logger_fd(log, "%", strlen("%"));
                p = last + 1;
                break;
            }

            case 'd': {
                size_t width;
                lts_str_t str;

                arg = (char const *)va_arg(args, long);
                width = long_width((long)arg);
                lts_str_init(&str, (uint8_t *)alloca(width), width);
                (void)lts_l2str(&str, (long)arg);
                (void)lts_write_logger_fd(log, str.data, str.len);
                p = last + 1;
                break;
            }

            default: {
                p = last;
                break;
            }
        }

        if (-1 == tmp_size) {
            total_size = -1;
            break;
        }

        total_size += tmp_size;
    }
    va_end(args);

    return total_size;
}
