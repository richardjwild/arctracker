#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "player.h"

void process_track_event_commands(const event_t *event, const player_instrument_t *instrument, player_track_t *track);

void process_global_event_commands(const event_t *events, int num_tracks, player_t *player);

bool is_tone_portamento(const event_t *event);

uint8_t get_note_delay(const event_t *);

uint8_t get_sample_slice(const event_t *);

#endif //ARCTRACKER_EFFECTS_H
