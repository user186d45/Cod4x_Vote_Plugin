cmake_minimum_required(VERSION 3.10)

project(Cod4x_Vote_Plugin VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#
# The CoD4x server is a 32-bit process, so the plugin MUST be built 32-bit.
# This requires the multilib toolchain on the build machine
# (e.g. `gcc-multilib` + 32-bit libc on Debian/Ubuntu).
# If you only have a 64-bit toolchain, you cannot produce a loadable plugin;
# set COD4X_32BIT_BUILD=OFF only for local syntax/warning checks.
#
option(COD4X_32BIT_BUILD "Build the 32-bit plugin required by the 32-bit CoD4x server" ON)

if(COD4X_32BIT_BUILD)
    add_compile_options(-m32)
    add_link_options(-m32)
endif()

add_compile_options(
    -Wall
    -Wextra
    -fvisibility=hidden        # export only PCL-marked symbols (no clash with the game binary)
)

#
# Plugins are shared objects loaded by the game's plugin handler.
#
add_library(${PROJECT_NAME} SHARED
    src/main.c
    src/timer.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

#
# - Threads::Threads    pthread_create/join + correct -pthread flags.
# - atomic              the vote struct uses atomic_ullong RMW ops; on 32-bit
#                       targets those lower to libatomic helper calls.
#
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    atomic
)

#
# The plugin handler loads the file by filename; drop the "lib" prefix so the
# artefact is Cod4x_Vote_Plugin.so.
#
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)