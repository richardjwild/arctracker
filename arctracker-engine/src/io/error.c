#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static bool error_occurred = false;
static char *error_message = NULL;

void error(char *error_message_p)
{
    error_message = error_message_p;
    error_occurred = true;
}

void error_with_detail(char *error_message_p, const char *detail)
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

#ifdef _WIN32
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
        fprintf(stderr, "Windows error %lu\n", (unsigned long)error);
        return;
    }
    fwprintf(stderr, L"Windows error %lu: %ls\n", (unsigned long)error, message);
}
#endif

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
