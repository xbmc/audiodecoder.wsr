cmake_minimum_required(VERSION 3.5)
project(in_wsr)

set(SOURCES nec/nec.c
            ws_audio.c
            ws_io.c
            ws_memory.c
            wsr_player.c)

add_library(in_wsr SHARED ${SOURCES})
