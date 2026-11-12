#include "error.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ERROR_MESSAGE_LENGTH 256

static bool error_occurred = false;
static char error_message[MAX_ERROR_MESSAGE_LENGTH];

#ifdef _WIN32
#include <windows.h>

void windows_error(DWORD error)
{
    wchar_t message[512];
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        0,
        message,
        sizeof message / sizeof message[0],
        NULL
    );
    if (length == 0)
    {
        snprintf(error_message, sizeof(error_message), "Unknown error %lu", (unsigned long) error);
        return;
    }
    snprintf(error_message, sizeof(error_message), L"%ls\n", message);
}

#endif

void error(const char *error_message_p)
{
    snprintf(error_message, sizeof(error_message), "%s", error_message_p);
    error_occurred = true;
}

void error_with_detail(const char *error_message_p, const char *detail)
{
    snprintf(error_message, sizeof(error_message), "%s [%s]", error_message_p, detail);
    error_occurred = true;
}

void system_error(const int system_errno)
{
    snprintf(error_message, sizeof(error_message), "%s", strerror(system_errno));
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
    error_message[0] = '\0';
}
