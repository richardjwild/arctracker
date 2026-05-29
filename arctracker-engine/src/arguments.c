#include <stddef.h>
#include <memory.h>
#include <stdio.h>
#include "arguments.h"
#include "arctracker-console.h"
#include "io/error.h"
#include "memory/heap.h"

static const char *ARG_HELP = "--help";
static const char *ARG_HELP_S = "-h";
static const char *ARG_OUTPUT = "--output=";
static const char *ARG_OUTPUT_S = "-o";

static const char *USAGE_MESSAGE =
        "Usage: arctracker [options] <modfile>\n"
        "\n"
        "Options are:\n"
        "\n"
        "\t-h or --help\n"
        "\t-c or --clip-warn\n"
        "\t-o<output file> or --output=<output file>\n"
        "\nArctracker version: " VERSION "\n"
        "\n";

bool handle_argument(args_t *, const char *);

args_t read_configuration(int p_argc, char *p_argv[])
{
    args_t config = {
        .mod_filename=NULL,
        .bounce=false,
        .output_filename=NULL,
    };
    if (p_argc > 1)
    {
        for (int i = 1; i < p_argc; i++)
        {
            if (!handle_argument(&config, p_argv[i]))
            {
                if (i == (p_argc - 1))
                    config.mod_filename = p_argv[i];
                else
                    error_with_detail("Unknown argument", p_argv[i]);
            }
        }
    }
    if (config.mod_filename == NULL)
    {
        error(USAGE_MESSAGE);
    }
    return config;
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

bool handle_argument(args_t *config, const char *arg)
{
    bool argument_was_handled = true;
    if (matches(arg, ARG_HELP, ARG_HELP_S))
    {
        printf("%s", USAGE_MESSAGE);
        exit(EXIT_SUCCESS);
    }
    else if (matches(arg, ARG_OUTPUT, ARG_OUTPUT_S))
    {
        const char *value = arg_value(arg, ARG_OUTPUT, ARG_OUTPUT_S);
        config->output_filename = (char *) value;
        config->bounce = true;
    }
    else
    {
        argument_was_handled = false;
    }
    return argument_was_handled;
}
