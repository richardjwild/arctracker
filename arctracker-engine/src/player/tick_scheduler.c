#include "tick_scheduler.h"

#include <stdio.h>

tick_scheduler_t tick_scheduler_create(const tempo_t tempo, const int sample_rate_in)
{
    if (tempo.actual_bpm > 0.0001) printf("Actual BPM: %0.3f\n", tempo.actual_bpm);
    const event_scheduler_t event_scheduler = {
        .ticks_per_event = tempo.ticks_per_event,
    };
    const audio_accumulator_t audio_accumulator = {
        .sample_rate = sample_rate_in,
        .ticks_per_second = tempo.ticks_per_second,
        .sample_fraction = fraction_create(0, tempo.ticks_per_second),
    };
    const tick_scheduler_t tick_scheduler = {
        .event_scheduler = event_scheduler,
        .audio_accumulator = audio_accumulator
    };
    return tick_scheduler;
}

void tick_scheduler_set_tempo(tick_scheduler_t *tick_scheduler, const tempo_t new_tempo)
{
    if (new_tempo.actual_bpm > 0.0001) printf("Actual BPM: %0.3f\n", new_tempo.actual_bpm);
    tick_scheduler->event_scheduler.ticks_per_event = new_tempo.ticks_per_event;
    tick_scheduler_set_ticks_per_second(tick_scheduler, new_tempo.ticks_per_second);
}

void tick_scheduler_set_ticks_per_second(tick_scheduler_t *tick_scheduler, const int new_ticks_per_second)
{
    tick_scheduler->audio_accumulator.ticks_per_second = new_ticks_per_second;
    // A tempo change discards up to one sample's worth of accumulated timing error:
    tick_scheduler->audio_accumulator.sample_fraction = fraction_create(0, new_ticks_per_second);
}

void tick_scheduler_restart(tick_scheduler_t *tick_scheduler)
{
    tick_scheduler->event_scheduler.just_started = true;
    tick_scheduler->event_scheduler.ticks = 0;
    tick_scheduler->event_scheduler.event_delay = 0;
    tick_scheduler->audio_accumulator.sample_fraction = fraction_create(0, tick_scheduler->audio_accumulator.ticks_per_second);
}

int tick_scheduler_samples_next_tick(audio_accumulator_t *accumulator)
{
    const fraction_t accumulated_samples = fraction_add_numerator(accumulator->sample_fraction, accumulator->sample_rate);
    // Since accumulated_samples is denominated in ticks per second, taking the whole part effectively divides the
    // sample rate (which is samples per second) plus the accumulated fraction by the ticks per second, which gives us
    // the whole number of samples per tick. In other words, the number of samples we need to write in the next tick.
    // Taking the fractional part gives us the fraction of a sample left over to be carried forward to the next tick.
    accumulator->sample_fraction = fractional_part(accumulated_samples);
    return whole_part(accumulated_samples);
}

void tick_scheduler_advance_tick(event_scheduler_t *event_scheduler)
{
    const int last_tick = event_scheduler->ticks_per_event - 1;
    int ticks = event_scheduler->ticks;
    if (ticks >= last_tick + event_scheduler->event_delay)
    {
        ticks = 0;
        event_scheduler->event_delay = 0;
    }
    else
    {
        ticks += 1;
    }
    event_scheduler->ticks = ticks;
    if (event_scheduler->just_started)
        event_scheduler->just_started = false;
}

bool tick_scheduler_is_new_event(const event_scheduler_t *event_scheduler)
{
    return !event_scheduler->just_started && event_scheduler->ticks == 0;
}

bool tick_scheduler_just_started(const event_scheduler_t *event_scheduler)
{
    return event_scheduler->just_started;
}
