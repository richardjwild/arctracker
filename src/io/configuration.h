#ifndef ARCTRACKER_CONFIGURATION_H
#define ARCTRACKER_CONFIGURATION_H

#include <stdbool.h>

typedef struct
{
    char *mod_filename;
    int volume;
    bool pianola;
    bool info;
    bool clip_warning;
    bool loop_forever;
    char *output_filename;
} args_t;

args_t configuration();

void read_configuration(int p_argc, char *p_argv[]);

#endif //ARCTRACKER_CONFIGURATION_H
