#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include <arctracker.h>
#include "play_mod.h"

void reset_arpeggiator(voice_t *voice);

void handle_effects_off_event(channel_event_t *event, voice_t *voice);

void handle_effects_on_event(channel_event_t *event, voice_t *voice);

#endif //ARCTRACKER_EFFECTS_H
