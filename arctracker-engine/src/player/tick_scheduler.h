#ifndef ARCTRACKER_CLOCK_H
#define ARCTRACKER_CLOCK_H

#include <stdbool.h>
#include "tempo.h"
#include "fraction/fraction.h"

typedef struct {
    int sample_rate;
    int ticks_per_second;
    fraction_t sample_fraction;
} audio_accumulator_t;

typedef struct {
    bool just_started;
    int ticks;
    int ticks_per_event;
    int event_delay;
} event_scheduler_t;

typedef struct {
    audio_accumulator_t audio_accumulator;
    event_scheduler_t event_scheduler;
} tick_scheduler_t;

tick_scheduler_t tick_scheduler_create(tempo_t tempo, int sample_rate_in);

void tick_scheduler_set_tempo(tick_scheduler_t *, tempo_t);

void tick_scheduler_set_ticks_per_second(tick_scheduler_t *tick_scheduler, int new_ticks_per_second);

void tick_scheduler_restart(tick_scheduler_t *);

int tick_scheduler_samples_next_tick(audio_accumulator_t *);

void tick_scheduler_advance_tick(event_scheduler_t *);

bool tick_scheduler_is_new_event(const event_scheduler_t *);

bool tick_scheduler_just_started(const event_scheduler_t *);

#endif //ARCTRACKER_CLOCK_H
