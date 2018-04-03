#include <stdio.h>
#include <stdlib.h>

void error(char* error_message)
{
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE);
}

void system_error(char* error_message)
{
    perror(error_message);
    exit(EXIT_FAILURE);
}