#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include <arctracker.h>
#include <playroutine/play_mod.h>

#define MAX_EFFECTS 4

void process_commands(channel_event_t *event, voice_t *voice, bool on_event);

#endif //ARCTRACKER_EFFECTS_H
