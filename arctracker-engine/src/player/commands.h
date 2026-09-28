#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "player.h"

void process_track_commands(const event_t *event, const player_instrument_t *instrument, player_track_t *track);

bool portamento(const event_t *event);

uint8_t get_note_delay(const event_t *);

uint8_t get_sample_slice(const event_t *);

void process_global_commands(const event_t *events, int num_tracks, player_t *player);

#endif //ARCTRACKER_EFFECTS_H
