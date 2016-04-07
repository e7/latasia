#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "buffer.h"
#include "protocol_lbm.h"


using std::cout;
using std::cerr;
using std::endl;


typedef struct {
    int cmnct_fd;
    uint32_t client_ip;
} thread_args_t;


extern "C"  {
    size_t lts_sys_pagesize = 4096;
}


void *rs_thread_proc(void *args)
{
#define BUF_SIZE        4096
    lts_pool_t *pool;
    lts_buffer_t *rbuf, *sbuf;
    thread_args_t *ta = reinterpret_cast<thread_args_t *>(args);

    pool = ::lts_create_pool(BUF_SIZE * 8);
    rbuf = ::lts_create_buffer(pool, BUF_SIZE, 0);
    sbuf = ::lts_create_buffer(pool, BUF_SIZE, 0);
    while (true) {
        ssize_t sz = ::recv(ta->cmnct_fd, rbuf->last, BUF_SIZE, 0);

        if (0 == sz) {
            cerr << "thread exit" << endl;
            break;
        }

        if (-1 == sz) {
            continue;
        }

        // 协议解析

        cerr << "recved data" << endl;
    }

    delete ta;
    ::lts_destroy_pool(pool);

    return NULL;
#undef BUF_SIZE
}


int main(int argc, char *argv[], char *env[])
{
    int exitcode;
    int lskt = -1;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;

    lskt = ::socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == lskt) {
        cerr << "socket() failed:" << errno << endl;
         return EXIT_FAILURE;
    }

    (void)::memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    my_addr.sin_port = ::htons(6742);
    if (-1 == ::bind(lskt, (struct sockaddr *)&my_addr, sizeof(my_addr))) {
        cerr << "bind() failed:" << errno << endl;
        (void)::close(lskt);
         return EXIT_FAILURE;
    }

    if (-1 == ::listen(lskt, SOMAXCONN)) {
        cerr << "listen() failed:" << errno << endl;
        (void)::close(lskt);
         return EXIT_FAILURE;
    }


    exitcode = EXIT_SUCCESS;
    while (true) {
        int fd;
        pthread_t pid;
        thread_args_t *ta;

        peer_addr_size = sizeof(peer_addr);
        fd = ::accept(lskt, (struct sockaddr *)&peer_addr, &peer_addr_size);
        if (-1 == fd) {
            continue;
        }

        ta = new thread_args_t();
        ta->cmnct_fd = fd;
        if (pthread_create(&pid, NULL, &rs_thread_proc, ta)) {
            exitcode = EXIT_FAILURE;
            break;
        }
    }

    return exitcode;
}
