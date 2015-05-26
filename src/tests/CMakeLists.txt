add_executable(test_lstack test_lstack.c)
add_executable(test_rbt_timer test_rbt_timer.c)

target_link_libraries(test_lstack utils mcheck)
target_link_libraries(test_rbt_timer utils mcheck)
