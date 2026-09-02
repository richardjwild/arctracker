#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "player.h"

bool portamento(const event_t *event);

void handle_effects_off_event(const event_t *event, voice_t *voice, player_t *);

void handle_effects_on_event(const event_t *event, voice_t *voice, player_t *);

#endif //ARCTRACKER_EFFECTS_H
