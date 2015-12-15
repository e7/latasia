/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "common.h"
#include "extra_errno.h"
#include "file.h"
#include "logger.h"

#define __THIS_FILE__       "src/file.c"


DEFINE_FILE_FD(lts_stderr_file, STDERR_FILENO, STDERR_NAME);
DEFINE_FILE(lts_log_file, ""); // 文件日志
DEFINE_FILE(lts_pid_file, ""); // pid文件


int lts_file_open(lts_file_t *file, int flags,
                  mode_t mode, void *logger)
{
    char *name;

    name = (char *)malloc(file->name.len + 1);
    (void)memcpy(name, file->name.data, file->name.len);
    name[file->name.len] = 0;

    file->fd = open(name, flags, mode);
    if (-1 == file->fd) {
        (void)lts_write_logger((lts_logger_t *)logger, LTS_LOG_ERROR,
                                "%s:open() %s failed: %s\n",
                                STR_LOCATION, name, lts_errno_desc[errno]);
        free(name);

        return -1;
    }
    file->rseek = 0;
    file->rseek = 0;
    free(name);

    return 0;
}


ssize_t lts_file_read(lts_file_t *file,
                      void *buf, size_t sz, void *logger)
{
    ssize_t rslt;

    rslt = pread(file->fd, buf, sz, file->rseek);
    if (-1 == rslt) {
        (void)lts_write_logger((lts_logger_t *)logger, LTS_LOG_ERROR,
                                "%s:pread(%d) failed: %s\n",
                                STR_LOCATION, file->fd, lts_errno_desc[errno]);

        return -1;
    }
    file->rseek += sz;

    return rslt;
}


ssize_t lts_file_write(lts_file_t *file,
                       void const *buf, size_t sz, void *logger)
{
    ssize_t rslt;

    rslt = pwrite(file->fd, buf, sz, file->wseek);
    if (-1 == rslt) {
        (void)lts_write_logger((lts_logger_t *)logger, LTS_LOG_ERROR,
                                "%s:pwrite(%d) failed: %s\n",
                                STR_LOCATION, file->fd, lts_errno_desc[errno]);

        return -1;
    }
    file->wseek += sz;

    return rslt;
}


void lts_file_close(lts_file_t *file)
{
    (void)close(file->fd);

    return;
}
