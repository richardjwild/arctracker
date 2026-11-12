#ifndef ARCTRACKER_ERROR_H
#define ARCTRACKER_ERROR_H

#include <stdbool.h>

#ifdef _WIN32

#include <windows.h>

void windows_error(DWORD error);

#endif

void error(const char *error_message_p);

void error_with_detail(const char *error_message_p, const char *detail);

void system_error(int system_errno);

bool has_error(void);

char *get_error_message(void);

void clear_error_state(void);

#endif // ARCTRACKER_ERROR_H
