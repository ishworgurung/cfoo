cmake_minimum_required(VERSION 3.17)
project(echo)

set(CMAKE_C_STANDARD 11)

include_directories(.)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

add_executable(echo
        CMakeLists.txt
        main.c defs.h)

#static linking
#target_link_libraries(liburing_web /usr/lib/liburing.a)

target_link_libraries(echo PUBLIC ${LIBURING_LIBRARIES})
target_include_directories(echo PUBLIC ${LIBURING_INCLUDE_DIRS})
target_compile_options(echo PUBLIC ${LIBURING_CFLAGS_OTHER})

#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ /usr/lib/liburing.a")