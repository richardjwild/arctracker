#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include "configuration.h"
#include "error.h"

static const char *ARG_PIANOLA = "--pianola";
static const char *ARG_INFO = "--info";
static const char *ARG_CLIP_WARN = "--clip-warn";
static const char *ARG_OSS = "--oss";
static const char *ARG_ALSA = "--alsa";
static const char *ARG_VOLUME = "--volume=";
static const char *ARG_LOOP = "--loop";

static args_t config = {
        .volume = 64,
        .pianola = false,
        .info = false,
        .clip_warning = false,
        .api = ALSA,
        .mod_filename = NULL,
        .loop_forever = false
};

inline
args_t configuration()
{
    return config;
}

void read_configuration(int p_argc, char *p_argv[])
{
    if (p_argc > 1)
    {
        for (int i = 1; i < p_argc; i++)
        {
            if (strcmp(p_argv[i], ARG_PIANOLA) == 0)
            {
                config.pianola = true;
            }
            else if (strcmp(p_argv[i], ARG_INFO) == 0)
            {
                config.info = true;
            }
            else if (strcmp(p_argv[i], ARG_CLIP_WARN) == 0)
            {
                config.clip_warning = true;
            }
            else if (strcmp(p_argv[i], ARG_LOOP) == 0)
            {
                config.loop_forever = true;
            }
            else if (strcmp(p_argv[i], ARG_OSS) == 0)
            {
                config.api = OSS;
            }
            else if (strcmp(p_argv[i], ARG_ALSA) == 0)
            {
                config.api = ALSA;
            }
            else if (strncmp(p_argv[i], ARG_VOLUME, strlen(ARG_VOLUME)) == 0)
            {
                p_argv[i] += strlen(ARG_VOLUME);
                config.volume = atoi(p_argv[i]);
                if (config.volume < 1 || config.volume > 256)
                    error("Volume must be between 1 and 256");
            }
            else if (i == (p_argc - 1))
            {
                config.mod_filename = p_argv[i];
            }
            else
                error_with_detail("Unknown argument", p_argv[i]);
        }
    }
    if (config.mod_filename == NULL)
    {
        error("Usage: arctracker [--loop] [--info] [--clip-warn] [--pianola] [--oss|--alsa] [--volume=<volume>] <modfile>");
    }
}