/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__FILE_H__
#define __LATASIA__FILE_H__


#include "list.h"
#include "adv_string.h"


typedef struct lts_file_s {
    int fd;
    lts_str_t name;
    off_t rseek;
    off_t wseek;
} lts_file_t;

#define DECLARE_FILE_FD(var, fd, name)      lts_file_t var = {\
                                                fd,\
                                                {\
                                                    (uint8_t *)(name),\
                                                    sizeof(name) - 1,\
                                                },\
                                                0, 0,\
                                            }
#define DECLARE_FILE(var, name)     DECLARE_FILE_FD(var, -1, name)


extern lts_file_t lts_stderr_file;
extern lts_file_t lts_log_file;


static inline void lts_file_rseek(lts_file_t *file, off_t ofst)
{
    file->rseek = ofst;
}
static inline void lts_file_wseek(lts_file_t *file, off_t ofst)
{
    file->wseek = ofst;
}
extern int lts_file_open(lts_file_t *file, int flags,
                         mode_t mode, void *logger);
extern ssize_t lts_file_read(lts_file_t *file, void *buf, size_t sz);
extern ssize_t lts_file_write(lts_file_t *file, void const *buf, size_t sz);
extern void lts_file_close(lts_file_t *file);
#endif // __LATASIA__FILE_H__
