cmake_minimum_required(VERSION 3.16.0)
project(cheap_protocol VERSION 1.0.0 LANGUAGES C)

set(SOURCES
    src/crc32.c
    src/essentials.c
    src/main.c
    src/message.c
)
add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(
    inc
)
target_precompile_headers(${PROJECT_NAME} PUBLIC inc/essentials.h)

