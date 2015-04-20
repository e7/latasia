add_subdirectory(modules)
add_library(utils adv_string.c mem_pool.c rbtree.c
                  socket.c buffer.c shmem.c logger.c
                  file.c conf.c vsignal.c rbt_timer.c)
