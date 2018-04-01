#include <stdio.h>
#include <stdlib.h>

void error(char* error_message)
{
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE);
}
