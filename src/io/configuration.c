#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "configuration.h"
#include "error.h"

static const char *ARG_HELP = "--help";
static const char *ARG_PIANOLA = "--pianola";
static const char *ARG_INFO = "--info";
static const char *ARG_CLIP_WARN = "--clip-warn";
static const char *ARG_VOLUME = "--volume=";
static const char *ARG_OUTPUT = "--output=";
static const char *ARG_VALUE_ALSA = "ALSA";
static const char *ARG_VALUE_OSS = "OSS";
static const char *ARG_LOOP = "--loop";
static const char *USAGE_MESSAGE =
        "Usage: arctracker [options] <modfile>\n"
        "\n"
        "Options are:\n"
        "\n"
        "\t--help\n"
        "\t--info\n"
        "\t--loop\n"
        "\t--clip-warn\n"
        "\t--pianola\n"
        "\t--output=<ALSA or OSS>\n"
        "\t--volume=<0 to 255>\n"
        "\n";

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
            if (strncmp(p_argv[i], ARG_HELP, strlen(ARG_HELP)) == 0)
            {
                printf("%s", USAGE_MESSAGE);
                exit(EXIT_SUCCESS);
            }
            else if (strncmp(p_argv[i], ARG_PIANOLA, strlen(ARG_PIANOLA)) == 0)
            {
                config.pianola = true;
            }
            else if (strncmp(p_argv[i], ARG_INFO, strlen(ARG_INFO)) == 0)
            {
                config.info = true;
            }
            else if (strncmp(p_argv[i], ARG_CLIP_WARN, strlen(ARG_CLIP_WARN)) == 0)
            {
                config.clip_warning = true;
            }
            else if (strncmp(p_argv[i], ARG_LOOP, strlen(ARG_LOOP)) == 0)
            {
                config.loop_forever = true;
            }
            else if (strncmp(p_argv[i], ARG_OUTPUT, strlen(ARG_OUTPUT)) == 0)
            {
                p_argv[i] += strlen(ARG_OUTPUT);
                if (strncmp(p_argv[i], ARG_VALUE_ALSA, strlen(ARG_VALUE_ALSA)) == 0)
                    config.api = ALSA;
                else if (strncmp(p_argv[i], ARG_VALUE_OSS, strlen(ARG_VALUE_OSS)) == 0)
                    config.api = OSS;
                else
                    error("Unrecognised output type. Try ALSA or OSS");
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
        error(USAGE_MESSAGE);
    }
}