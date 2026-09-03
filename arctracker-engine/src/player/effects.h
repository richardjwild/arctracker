#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "player.h"

bool portamento(const event_t *event);

void handle_effects_before_note(const event_t *event, const voice_t *voice, player_t *);

uint8_t get_note_delay(const event_t *);

uint8_t get_sample_slice(const event_t *);

void handle_effects_on_event(const event_t *event, voice_t *voice, player_t *);

void handle_effects_off_event(const event_t *event, voice_t *voice, player_t *);

#endif //ARCTRACKER_EFFECTS_H
