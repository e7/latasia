add_executable(latasia latasia.c
               mod_core.c mod_event_core.c mod_event_epoll.c
               mod_app_http_core.c)
target_link_libraries(latasia utils z mcheck)
