add_executable(server server.cpp)
target_link_libraries(server PRIVATE fmt::fmt-header-only)

add_executable(test_server test_server.cpp)
target_link_libraries(test_server PRIVATE fmt::fmt-header-only)