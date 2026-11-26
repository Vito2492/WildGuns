# WildGuns for Android: SDL2 (sources in jni/SDL, put there by
# tools/android-prepare.sh) and the program, built as libmain.so, which
# SDL's Java side loads and starts.
cmake_minimum_required(VERSION 3.18)
project(WildGuns C)

add_subdirectory(SDL)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
add_library(main SHARED ${ROOT}/src/wildguns.c)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include ${ROOT}/src)
target_compile_options(main PRIVATE -O2 -Wno-unused-parameter)
target_link_libraries(main SDL2 m dl log android)
