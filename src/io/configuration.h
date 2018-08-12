#ifndef ARCTRACKER_CONFIGURATION_H
#define ARCTRACKER_CONFIGURATION_H

#include <stdbool.h>

#define ARG_PIANOLA "--pianola"
#define ARG_INFO "--info"
#define ARG_CLIP_WARN "--clip-warn"
#define ARG_OSS "--oss"
#define ARG_ALSA "--alsa"
#define ARG_VOLUME "--volume="
#define ARG_LOOP "--loop"

typedef enum
{
    NOT_SPECIFIED, OSS, ALSA
} output_api;

typedef struct
{
    char *mod_filename;
    int volume;
    bool pianola;
    bool info;
    bool clip_warning;
    output_api api;
    bool loop_forever;
} args_t;

args_t configuration();

void read_configuration(int p_argc, char *p_argv[]);

#endif //ARCTRACKER_CONFIGURATION_H
