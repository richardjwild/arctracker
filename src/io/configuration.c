#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "configuration.h"
#include "error.h"

static const char *ARG_HELP = "--help";
static const char *ARG_HELP_S = "-h";
static const char *ARG_PIANOLA = "--pianola";
static const char *ARG_PIANOLA_S = "-p";
static const char *ARG_INFO = "--info";
static const char *ARG_INFO_S = "-i";
static const char *ARG_CLIP_WARN = "--clip-warn";
static const char *ARG_CLIP_WARN_S = "-c";
static const char *ARG_VOLUME = "--volume=";
static const char *ARG_VOLUME_S = "-v";
static const char *ARG_LOOP = "--loop";
static const char *ARG_LOOP_S = "-l";
static const char *ARG_OUTPUT = "--output=";
static const char *ARG_OUTPUT_S = "-o";

static const char *USAGE_MESSAGE =
        "Usage: arctracker [options] <modfile>\n"
        "\n"
        "Options are:\n"
        "\n"
        "\t-h or --help\n"
        "\t-i or --info\n"
        "\t-l or --loop\n"
        "\t-c or --clip-warn\n"
        "\t-p or --pianola\n"
        "\t-v<1 to 255> or --volume=<1 to 255>\n"
        "\n";

static args_t config = {
        .volume = 64,
        .pianola = false,
        .info = false,
        .clip_warning = false,
        .mod_filename = NULL,
        .loop_forever = false,
        .output_filename = NULL
};

bool handle_argument(const char *);

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
            if (!handle_argument(p_argv[i]))
            {
                if (i == (p_argc - 1))
                    config.mod_filename = p_argv[i];
                else
                    error_with_detail("Unknown argument", p_argv[i]);
            }
        }
    }
    if (config.mod_filename == NULL)
        error(USAGE_MESSAGE);
}

bool matches(const char *arg, const char *long_form, const char *short_form)
{
    return strncmp(arg, long_form, strlen(long_form)) == 0
           || strncmp(arg, short_form, strlen(short_form)) == 0;
}

const char *arg_value(const char *arg, const char *long_form, const char *short_form)
{
    if (strncmp(arg, long_form, strlen(long_form)) == 0)
        return arg + strlen(long_form);
    else
        return arg + strlen(short_form);
}

bool handle_argument(const char *arg)
{
    bool argument_was_handled = true;
    if (matches(arg, ARG_HELP, ARG_HELP_S))
    {
        printf("%s", USAGE_MESSAGE);
        exit(EXIT_SUCCESS);
    }
    else if (matches(arg, ARG_PIANOLA, ARG_PIANOLA_S))
    {
        config.pianola = true;
    }
    else if (matches(arg, ARG_INFO, ARG_INFO_S))
    {
        config.info = true;
    }
    else if (matches(arg, ARG_CLIP_WARN, ARG_CLIP_WARN_S))
    {
        config.clip_warning = true;
    }
    else if (matches(arg, ARG_LOOP, ARG_LOOP_S))
    {
        config.loop_forever = true;
    }
    else if (matches(arg, ARG_VOLUME, ARG_VOLUME_S))
    {
        const char *value = arg_value(arg, ARG_VOLUME, ARG_VOLUME_S);
        config.volume = atoi(value);
        if (config.volume < 1 || config.volume > 256)
            error("Volume must be a number between 1 and 256");
    }
    else if (matches(arg, ARG_OUTPUT, ARG_OUTPUT_S))
    {
        const char *value = arg_value(arg, ARG_OUTPUT, ARG_OUTPUT_S);
        config.output_filename = (char *) value;
    }
    else
    {
        argument_was_handled = false;
    }
    return argument_was_handled;
}