project(audiodecoder.wsr)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(p8-platform REQUIRED)

include(ExternalProject)

include_directories(${KODI_INCLUDE_DIR}/..
                    ${p8platform_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/lib/in_wsr)

add_subdirectory(lib/in_wsr)

set(WSR_SOURCES src/WSRCodec.cpp)

add_definitions(-DLIBRARY_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}"
                -DLIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")

set(WSR_ADDITIONAL_BINARY $<TARGET_FILE:in_wsr>)

set(DEPLIBS ${p8-platform_LIBRARIES})

build_addon(audiodecoder.wsr WSR DEPLIBS)

include(CPack)
