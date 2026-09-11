#ifndef ARCTRACKER_EFFECTS_H
#define ARCTRACKER_EFFECTS_H

#include "player.h"

void process_instrument_effects(const event_t *event, const player_instrument_t *instrument, player_track_t *track, audio_generator_t *generator);

bool portamento(const event_t *event);

void handle_effects_before_note(const event_t *event, const player_track_t *track, player_t *);

uint8_t get_note_delay(const event_t *);

uint8_t get_sample_slice(const event_t *);

void process_non_instrument_effects(const event_t *event, audio_channel_t *channel, player_track_t *, player_t *);

#endif //ARCTRACKER_EFFECTS_H
