cmake_minimum_required(VERSION 3.22.1)
project(wildguns C)

# SDL2 source directory is provided by CI / local setup via -DSDL2_DIR=... or vendor/SDL2
if(NOT DEFINED SDL2_DIR)
    set(SDL2_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../vendor/SDL2")
endif()

add_subdirectory("${SDL2_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/SDL2")

add_library(main SHARED
    "${CMAKE_CURRENT_LIST_DIR}/../../../../src/wildguns.c"
)

target_include_directories(main PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../../src"
)

target_link_libraries(main
    SDL2
    dl
    log
    m
)
