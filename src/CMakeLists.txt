ADD_SUBDIRECTORY(modules)

AUX_SOURCE_DIRECTORY(. src_utils)
ADD_EXECUTABLE(latasia ${src_utils})
TARGET_LINK_LIBRARIES(latasia z mcheck lts_modules)

ADD_EXECUTABLE(test_charmap tests/test_charmap.c)
TARGET_LINK_LIBRARIES(test_charmap mcheck)

ADD_EXECUTABLE(test_adv_string adv_string.c base64.c mem_pool.c tests/test_adv_string.c)
TARGET_LINK_LIBRARIES(test_adv_string mcheck)

ADD_EXECUTABLE(test_lstack tests/test_lstack.c)
TARGET_LINK_LIBRARIES(test_lstack mcheck)

ADD_EXECUTABLE(test_rbt_timer rbtree.c rbmap.c rbt_timer.c tests/test_rbt_timer.c)
TARGET_LINK_LIBRARIES(test_rbt_timer mcheck)

ADD_EXECUTABLE(press_tcp tests/press_tcp.c)
TARGET_LINK_LIBRARIES(press_tcp mcheck pthread)

ADD_EXECUTABLE(test_sjson adv_string.c simple_json.c base64.c
                          mem_pool.c rbtree.c tests/test_sjson.c)
TARGET_LINK_LIBRARIES(test_sjson mcheck)

ADD_EXECUTABLE(test_rbmap adv_string.c simple_json.c base64.c
                          mem_pool.c rbtree.c rbmap.c tests/test_rbmap.c)
TARGET_LINK_LIBRARIES(test_rbmap mcheck)

ADD_EXECUTABLE(test_obj_pool tests/test_obj_pool.c obj_pool.c)
TARGET_LINK_LIBRARIES(test_obj_pool mcheck)

#remove_definitions(-std=gnu99)
SET_SOURCE_FILES_PROPERTIES(tests/fake_server.cc PROPERTIES LANGUAGE CXX)
ADD_EXECUTABLE(fake_server protocol_sjsonb.c buffer.c mem_pool.c adv_string.c
                           rbtree.c simple_json.c base64.c tests/fake_server.cc)
TARGET_LINK_LIBRARIES(fake_server mcheck pthread)
