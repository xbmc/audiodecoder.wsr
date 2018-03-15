project(in_wsr)

enable_language(C)

cmake_minimum_required(VERSION 2.6)

set(SOURCES nec/nec.c
            ws_audio.c
            ws_io.c
            ws_memory.c
            wsr_player.c)

add_library(in_wsr SHARED ${SOURCES})
