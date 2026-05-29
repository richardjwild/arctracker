#ifndef ARCTRACKER_CONFIGURATION_H
#define ARCTRACKER_CONFIGURATION_H

#include <stdbool.h>

typedef struct
{
    char *mod_filename;
    bool bounce;
    char *output_filename;
} args_t;

args_t read_configuration(int p_argc, char *p_argv[]);

#endif //ARCTRACKER_CONFIGURATION_H
