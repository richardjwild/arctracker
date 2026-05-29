#include "tick_scheduler.h"

static const int DEFAULT_TICKS_PER_SECOND = 50;

tick_scheduler_t tick_scheduler_create(const int initial_ticks_per_event, const int sample_rate_in)
{
    event_scheduler_t event_scheduler = {
        .ticks = 0,
        .ticks_per_event = initial_ticks_per_event
    };
    audio_accumulator_t audio_accumulator = {
        .sample_rate = sample_rate_in,
        .accumulator = 0,
        .ticks_per_second = DEFAULT_TICKS_PER_SECOND
    };
    tick_scheduler_t tick_scheduler = {
        .event_scheduler = event_scheduler,
        .audio_accumulator = audio_accumulator
    };
    return tick_scheduler;
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
