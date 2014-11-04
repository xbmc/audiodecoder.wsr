project(in_wsr)

enable_language(C)

cmake_minimum_required(VERSION 2.6)

set(SOURCES nec/nec.c
            ws_audio.c
            ws_io.c
            ws_memory.c
            wsr_player.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

add_library(in_wsr STATIC ${SOURCES})
