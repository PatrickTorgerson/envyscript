add_executable(cogtest "main.c")

target_include_directories(cogtest PRIVATE "../source")

target_link_libraries(cogtest coglib)