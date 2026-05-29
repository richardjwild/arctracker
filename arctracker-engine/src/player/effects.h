#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "arctracker-console.h"
#include "player.h"

void reset_arpeggiator(voice_t *voice);

bool portamento(const event_t *event);

void handle_effects_off_event(const event_t *event, voice_t *voice);

void handle_effects_on_event(const event_t *event, voice_t *voice, player_t *);

#endif //ARCTRACKER_EFFECTS_H
