#ifndef ARCTRACKER_CONSOLE_H
#define ARCTRACKER_CONSOLE_H

#include <arctracker.h>

void configure_console(bool pianola, const module_t *module);

void output_to_console(const channel_event_t *line);

void write_info(const module_t module);

void warn_clip();

#endif //ARCTRACKER_CONSOLE_H
