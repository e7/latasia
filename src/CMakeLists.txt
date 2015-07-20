AUX_SOURCE_DIRECTORY(. src_utils)
AUX_SOURCE_DIRECTORY(modules src_modules)
ADD_EXECUTABLE(latasia ${src_utils} ${src_modules})
TARGET_LINK_LIBRARIES(latasia z mcheck pthread)

ADD_EXECUTABLE(test_lstack tests/test_lstack.c)
TARGET_LINK_LIBRARIES(test_lstack mcheck)

ADD_EXECUTABLE(test_rbt_timer rbtree.c rbt_timer.c tests/test_rbt_timer.c)
TARGET_LINK_LIBRARIES(test_rbt_timer mcheck)

ADD_EXECUTABLE(press_tcp tests/press_tcp.c)
TARGET_LINK_LIBRARIES(press_tcp mcheck)
