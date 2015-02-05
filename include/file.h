#ifndef __EVIEO__FILE_H__
#define __EVIEO__FILE_H__


#include "adv_string.h"


typedef struct lts_file_s {
    int fd;
    lts_str_t name;
} lts_file_t;


extern lts_file_t lts_stderr_file;
extern lts_file_t lts_log_file;


extern int lts_file_open(lts_file_t *file, int flags,
                         mode_t mode, void *logger);
extern ssize_t lts_file_read(lts_file_t *file,
                             void *buf, size_t sz, off_t ofst);
extern ssize_t lts_file_write(lts_file_t *file,
                             void const *buf, size_t sz, off_t ofst);
extern void lts_file_close(lts_file_t *file);
#endif // __EVIEO__FILE_H__
