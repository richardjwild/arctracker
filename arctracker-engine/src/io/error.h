#ifndef ARCTRACKER_ERROR_H
#define ARCTRACKER_ERROR_H

#include <stdbool.h>

void error(const char *error_message);

void error_with_detail(const char *error_message, const char *detail);

void system_error(int system_errno);

bool has_error(void);

char *get_error_message(void);

bool clear_error_state(void);

#endif // ARCTRACKER_ERROR_H
