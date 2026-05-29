#ifndef ARCTRACKER_CLOCK_H
#define ARCTRACKER_CLOCK_H

#include <stdbool.h>

typedef struct {
    int accumulator;
    int ticks_per_second;
    int sample_rate;
} audio_accumulator_t;

typedef struct {
    int ticks;
    int ticks_per_event;
} event_scheduler_t;

typedef struct {
    audio_accumulator_t audio_accumulator;
    event_scheduler_t event_scheduler;
} tick_scheduler_t;

tick_scheduler_t tick_scheduler_create(int initial_ticks_per_event, int sample_rate_in);

void tick_scheduler_restart(tick_scheduler_t *);

void tick_scheduler_accumulate(audio_accumulator_t *);

int tick_scheduler_samples_to_write(const audio_accumulator_t *);

void tick_scheduler_consume_samples(audio_accumulator_t *);

void tick_scheduler_advance_tick(event_scheduler_t *);

bool tick_scheduler_is_new_event(const event_scheduler_t *);

#endif //ARCTRACKER_CLOCK_H
