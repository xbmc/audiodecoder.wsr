project(audiodecoder.wsr)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)

include(ExternalProject)

include_directories(${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/in_wsr)

add_subdirectory(lib/in_wsr)

set(WSR_SOURCES src/WSRCodec.cpp)

set(DEPLIBS -Wl,--whole-archive in_wsr -Wl,--no-whole-archive)

build_addon(audiodecoder.wsr WSR DEPLIBS)

include(CPack)
