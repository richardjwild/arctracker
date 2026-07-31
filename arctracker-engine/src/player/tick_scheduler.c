#include "tick_scheduler.h"

#include <stdio.h>

tick_scheduler_t tick_scheduler_create(const tempo_t tempo, const int sample_rate_in)
{
    if (tempo.actual_bpm > 0.0001) printf("Actual BPM: %0.3f\n", tempo.actual_bpm);
    event_scheduler_t event_scheduler = {
        .ticks = 0,
        .ticks_per_event = tempo.ticks_per_event,
    };
    audio_accumulator_t audio_accumulator = {
        .sample_rate = sample_rate_in,
        .accumulator = 0,
        .ticks_per_second = tempo.ticks_per_second,
    };
    tick_scheduler_t tick_scheduler = {
        .event_scheduler = event_scheduler,
        .audio_accumulator = audio_accumulator
    };
    return tick_scheduler;
}

void tick_scheduler_set_tempo(tick_scheduler_t *tick_scheduler, const tempo_t new_tempo)
{
    if (new_tempo.actual_bpm > 0.0001) printf("Actual BPM: %0.3f\n", new_tempo.actual_bpm);
    tick_scheduler->event_scheduler.ticks_per_event = new_tempo.ticks_per_event;
    tick_scheduler->audio_accumulator.ticks_per_second = new_tempo.ticks_per_second;
}

void tick_scheduler_restart(tick_scheduler_t *tick_scheduler)
{
    tick_scheduler->event_scheduler.ticks = 1;
    tick_scheduler->audio_accumulator.accumulator = 0;
}

void tick_scheduler_accumulate(audio_accumulator_t *audio_accumulator)
{
    audio_accumulator->accumulator += audio_accumulator->sample_rate;
}

int tick_scheduler_samples_to_write(const audio_accumulator_t *audio_accumulator)
{
    return audio_accumulator->accumulator / audio_accumulator->ticks_per_second;
}

void tick_scheduler_consume_samples(audio_accumulator_t *audio_accumulator)
{
    int accumulator = audio_accumulator->accumulator;
    accumulator %= audio_accumulator->ticks_per_second;
    audio_accumulator->accumulator = accumulator;
}

void tick_scheduler_advance_tick(event_scheduler_t *event_scheduler)
{
    const int ticks_per_event = event_scheduler->ticks_per_event;
    int ticks = event_scheduler->ticks;
    ticks += 1;
    if (ticks >= ticks_per_event)
        ticks -= ticks_per_event;
    event_scheduler->ticks = ticks;
}

bool tick_scheduler_is_new_event(const event_scheduler_t *event_scheduler)
{
    return (event_scheduler->ticks == 0);
}
