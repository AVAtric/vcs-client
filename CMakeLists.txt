cmake_minimum_required(VERSION 3.12)
project(vcs-client C)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES main.c simple_message_client_commandline_handling.c)

#add_executable(vcs-client main.c)
add_executable(vcs-client ${SOURCE_FILES})