/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "common.h"
#include "file.h"
#include "logger.h"


DECLARE_FILE_FD(lts_stderr_file, STDERR_FILENO, STDERR_NAME);
DECLARE_FILE(lts_log_file, "latasia.log"); // 文件日志


int lts_file_open(lts_file_t *file, int flags,
                  mode_t mode, void *logger)
{
    char *buf;

    buf = (char *)alloca(file->name.len + 1);
    (void)memcpy(buf, file->name.data, file->name.len);
    buf[file->name.len] = 0;

    file->fd = open(buf, flags, mode);
    if (-1 == file->fd) {
        (void)lts_write_logger(
            (lts_logger_t *)logger, LTS_LOG_ERROR,
            "open %s failed: %s\n", buf, strerror(errno)
        );
        return -1;
    }
    file->rseek = 0;
    file->rseek = 0;

    return 0;
}


ssize_t lts_file_read(lts_file_t *file, void *buf, size_t sz)
{
    ssize_t rslt;

    rslt = pread(file->fd, buf, sz, file->rseek);
    if (-1 == rslt) {
        return -1;
    }
    file->rseek += sz;

    return rslt;
}


ssize_t lts_file_write(lts_file_t *file, void const *buf, size_t sz)
{
    ssize_t rslt;

    rslt = pwrite(file->fd, buf, sz, file->wseek);
    if (-1 == rslt) {
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
