add_definitions(-D_GNU_SOURCE)

add_executable(http_server01 http_server01.c)
target_link_libraries(http_server01 yolanda)

add_executable(http_server_test http_server_test.c)
target_link_libraries(http_server_test yolanda)
