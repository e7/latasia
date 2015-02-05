#ifndef __EVIEO__LOGGER_H__
#define __EVIEO__LOGGER_H__


#include "file.h"


enum {
    LTS_DEBUG = 1,
    LTS_INFO,
    LTS_NOTICE,
    LTS_WARN,
    LTS_ERROR,
    LTS_CRIT,
    LTS_ALERT,
    LTS_EMERGE,
};


typedef struct lts_logger_s {
    lts_file_t *file;
} lts_logger_t;


extern lts_logger_t lts_stderr_logger;
extern lts_logger_t lts_file_logger;


extern ssize_t lts_write_logger_fd(lts_logger_t *log,
                                   void const *buf, size_t n);
extern ssize_t lts_write_logger(lts_logger_t *log,
                                int level, char const *fmt, ...);
#endif // __EVIEO__LOGGER_H__
