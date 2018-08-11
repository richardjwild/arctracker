#ifndef ARCTRACKER_ERROR_H
#define ARCTRACKER_ERROR_H

#define MALLOC_FAILED "Could not allocate memory"

void error(const char *error_message);

void error_with_detail(const char *error_message, const char *detail);

void system_error(const char *error_message);

#endif // ARCTRACKER_ERROR_H