#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool error_occurred = false;
static char *error_message = NULL;

void error(char *error_message_p)
{
    error_message = error_message_p;
    error_occurred = true;
}

void error_with_detail(const char *error_message_p, const char *detail)
{
    static char message_buffer[256];
    error_occurred = true;
    if (snprintf(message_buffer, 256, "%s [%s]", error_message_p, detail) < 256)
        error_message = message_buffer;
    else
        error_message = error_message_p;
}

void system_error(int system_errno)
{
    error_message = strerror(system_errno);
    error_occurred = true;
}

bool has_error(void)
{
    return error_occurred;
}

char *get_error_message(void)
{
    return error_message;
}

void clear_error_state(void)
{
    error_occurred = false;
    error_message = NULL;
}
