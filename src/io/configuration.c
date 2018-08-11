#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <io/configuration.h>
#include <io/error.h>

static args_t config = {
        .volume = 64,
        .pianola = false,
        .api = ALSA,
        .mod_filename = NULL,
        .loop_forever = false
};

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
        error("Usage: arctracker [--loop] [--pianola] [--oss|--alsa] [--volume=<volume>] <modfile>");
    }
}