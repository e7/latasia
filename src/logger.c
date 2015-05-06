/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <time.h>
#include "file.h"
#include "logger.h"


extern pid_t lts_pid;
extern int64_t lts_current_time;


lts_str_t LTS_LOG_PREFIXES[LTS_LOG_MAX];

lts_logger_t lts_stderr_logger = {
    &lts_stderr_file,
    LTS_LOG_NOTICE,
};
lts_logger_t lts_file_logger = {
    &lts_log_file,
    LTS_LOG_INFO,
};


ssize_t lts_write_logger_fd(lts_logger_t *log, void const *buf, size_t n)
{
    return lts_file_write(log->file, buf, n);
}


ssize_t lts_write_logger(lts_logger_t *log,
                         int level, char const *fmt, ...)
{
    va_list args;
    int len, total;
    char buf[4096];
    struct tm tmp_tm;
    time_t current_time;

    if (level < log->level) {
        return 0;
    }

    // 时间转换
    current_time = (time_t)(lts_current_time / 10);
    (void)gmtime_r(&current_time, &tmp_tm);
    total = len = snprintf(
        buf, sizeof(buf), "%d.%02d.%02d %02d:%02d:%02d [%d] [%s] ",
        1900 + tmp_tm.tm_year, 1 + tmp_tm.tm_mon, tmp_tm.tm_mday,
        tmp_tm.tm_hour, tmp_tm.tm_min, tmp_tm.tm_sec,
        lts_pid, LTS_LOG_PREFIXES[level].data
    );
    va_start(args, fmt);
    len = vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    va_end(args);

    if (len > 0) {
        total += len;
        return lts_write_logger_fd(log, buf, total);
    } else {
        return -1;
    }
}

/*
ssize_t lts_write_logger(lts_logger_t *log,
                         int level, char const *fmt, ...)
{
    va_list args;
    char const *p, *last;
    char const *arg;
    lts_str_t const *prefix;
    ssize_t total_size, tmp_size;
    char delimiter = 0x20;

    if (level < log->level) {
        return 0;
    }

    prefix = LTS_LOG_PREFIXES + level;
    (void)lts_write_logger_fd(log, prefix->data, prefix->len);
    (void)lts_write_logger_fd(log, &delimiter, 1);
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
*/
