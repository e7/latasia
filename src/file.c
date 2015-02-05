#include "common.h"
#include "file.h"
#include "logger.h"


lts_file_t lts_stderr_file = {
    STDERR_FILENO,
    {
        (uint8_t *)STDERR_NAME,
        sizeof(STDERR_NAME) - 1,
    },
};
lts_file_t lts_log_file; // 日志文件


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
            (lts_logger_t *)logger, LTS_ERROR,
            "open %s failed: %s\n", buf, strerror(errno)
        );
        return -1;
    }

    return 0;
}


ssize_t lts_file_read(lts_file_t *file, void *buf, size_t sz, off_t ofst)
{
    int tmp_err;
    ssize_t rslt;

    rslt = pread(file->fd, buf, sz, ofst);
    tmp_err = errno;
    if ((-1 == rslt) && (ESPIPE == tmp_err)) {
        rslt = read(file->fd, buf, sz);
    }

    return rslt;
}


ssize_t lts_file_write(lts_file_t *file,
                       void const *buf, size_t sz, off_t ofst)
{
    int tmp_err;
    ssize_t rslt;

    rslt = pwrite(file->fd, buf, sz, ofst);
    tmp_err = errno;
    if ((-1 == rslt) && (ESPIPE == tmp_err)) {
        rslt = write(file->fd, buf, sz);
    }

    return rslt;
}


void lts_file_close(lts_file_t *file)
{
    (void)close(file->fd);

    return;
}
