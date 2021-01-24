#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
/*
 * Utility function to convert a string to lower case.
 * */
void strtolower(char *str) {
    for (; *str; ++str) {
        *str = (char)tolower(*str);
    }
}

// One function that prints the system
// call and the error details and then
// exits with error code 1.
// Non-zero meaning things didn't go well.
void fatal_error(const char *syscall) {
    perror(syscall);
    exit(1);
}


/*
 * Helper function for cleaner looking code.
 * */
void *httpd_malloc(size_t size) {
    void *buf = malloc(size);
    if (!buf) {
        fatal_error("malloc");
    }
    printf("allocated %zu bytes\n", size);
    return buf;
}
