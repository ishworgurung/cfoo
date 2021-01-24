#include <unistd.h>
#include <stdlib.h>

/*
 * Utility function to convert a string to lower case.
 * */
void strtolower(char*);

// One function that prints the system
// call and the error details and then
// exits with error code 1.
// Non-zero meaning things didn't go well.
void fatal_error(const char*);


/*
 * Helper function for cleaner looking code.
 * */
void *httpd_malloc(size_t size);
