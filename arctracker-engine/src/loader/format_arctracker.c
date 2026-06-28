#include "format_arctracker.h"

#include "io/error.h"

static bool is_arctracker_module(mapped_file_t);
static module_t *read_arctracker_module(mapped_file_t);
static bool write_arctracker_module(const module_t *, FILE *);

format_t arctracker_format(void)
{
    return (format_t) {
        .is_this_format = is_arctracker_module,
        .read_module = read_arctracker_module,
        .write_module = write_arctracker_module,
    };
}

static bool is_arctracker_module(mapped_file_t mapped_file)
{
    // TODO: Everything!
    return false;
}

static module_t *read_arctracker_module(mapped_file_t mapped_file)
{
    // TODO: Everything!
    return NULL;
}

static bool write_arctracker_module(const module_t *module, FILE *fp)
{
    // TODO: Everything!
    error("Saving native modules is not implemented yet");
    return false;
}
