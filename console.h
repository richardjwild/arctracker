#ifndef ARCTRACKER_CONSOLE_H
#define ARCTRACKER_CONSOLE_H

void configure_console(const bool pianola, const module_t *module);

void output_new_position(const positions_t *positions);

void pianola_roll(const positions_t *positions, const channel_event_t *line);

#endif //ARCTRACKER_CONSOLE_H
