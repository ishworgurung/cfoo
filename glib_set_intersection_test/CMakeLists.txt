cmake_minimum_required(VERSION 3.15)
project(glib_set_intersection_test C)

set(CMAKE_C_STANDARD 11)
find_package (PkgConfig REQUIRED)
pkg_check_modules (GLIB2 REQUIRED glib-2.0)
include_directories (${GLIB2_INCLUDE_DIRS})
link_directories (${GLIB2_LIBRARY_DIRS})
add_executable(glib_set_intersection_test main.c)
add_definitions (${GLIB2_CFLAGS_OTHER})
target_link_libraries (glib_set_intersection_test ${GLIB2_LIBRARIES})

