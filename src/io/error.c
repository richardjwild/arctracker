#include <stdio.h>
#include <stdlib.h>

void error(const char *error_message)
{
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE);
}

void error_with_detail(const char *error_message, const char *detail)
{
    fprintf(stderr, "%s (%s)\n", error_message, detail);
    exit(EXIT_FAILURE);
}

void system_error(const char *error_message)
{
    perror(error_message);
    exit(EXIT_FAILURE);
}