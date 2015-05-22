/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__LOGGER_H__
#define __LATASIA__LOGGER_H__


#include "adv_string.h"
#include "file.h"

#define LOG_SHOW_LOC        TRUE


enum {
    LTS_LOG_DEBUG = 1,
    LTS_LOG_INFO,
    LTS_LOG_NOTICE,
    LTS_LOG_WARN,
    LTS_LOG_ERROR,
    LTS_LOG_CRIT,
    LTS_LOG_ALERT,
    LTS_LOG_EMERGE,
    LTS_LOG_MAX,
};


typedef struct lts_logger_s {
    lts_file_t *file;
    int level;
} lts_logger_t;


extern lts_str_t LTS_LOG_PREFIXES[LTS_LOG_MAX];
#define lts_init_log_prefixes() do {\
    LTS_LOG_PREFIXES[LTS_LOG_DEBUG] = (lts_str_t)lts_string("DEBUG");\
    LTS_LOG_PREFIXES[LTS_LOG_INFO] = (lts_str_t)lts_string("INFO");\
    LTS_LOG_PREFIXES[LTS_LOG_NOTICE] = (lts_str_t)lts_string("NOTICE");\
    LTS_LOG_PREFIXES[LTS_LOG_WARN] = (lts_str_t)lts_string("WARN");\
    LTS_LOG_PREFIXES[LTS_LOG_ERROR] = (lts_str_t)lts_string("ERROR");\
    LTS_LOG_PREFIXES[LTS_LOG_CRIT] = (lts_str_t)lts_string("CRIT");\
    LTS_LOG_PREFIXES[LTS_LOG_ALERT] = (lts_str_t)lts_string("ALERT");\
    LTS_LOG_PREFIXES[LTS_LOG_EMERGE] = (lts_str_t)lts_string("EMERGE");\
} while (0)
extern lts_logger_t lts_stderr_logger;
extern lts_logger_t lts_file_logger;


extern ssize_t lts_write_logger_fd(lts_logger_t *log,
                                   void const *buf, size_t n);
extern ssize_t lts_write_logger(lts_logger_t *log,
                                int level, char const *fmt, ...);

#if LOG_SHOW_LOC
#define STR_LOCATION        __FILE__":"SRC2STR(__LINE__)
#else
#define STR_LOCATION        "*"
#endif // LOG_SHOW_LOC
#endif // __LATASIA__LOGGER_H__
