#ifndef ARCTRACKER_CONSOLE_H
#define ARCTRACKER_CONSOLE_H

void configure_console(bool pianola, const module_t *module);

void output_new_position();

void pianola_roll(const channel_event_t *line);

#endif //ARCTRACKER_CONSOLE_H
