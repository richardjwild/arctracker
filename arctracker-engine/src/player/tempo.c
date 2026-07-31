#include "tempo.h"
#include <math.h>

#define IDEAL_TICKS_PER_SECOND 50
#define TICKS_PER_SECOND_RANGE 30

static void order_ticks_per_second(int *);
static tempo_t find_best_fit(int, int, const int *);

void calculate_tempo(tempo_t *tempo_array, const int lines_per_beat, const int max_bpm)
{
    int ticks_per_second[TICKS_PER_SECOND_RANGE];
    order_ticks_per_second(ticks_per_second);
    for (int desired_bpm = 1; desired_bpm <= max_bpm; desired_bpm++)
    {
        tempo_array[desired_bpm] = find_best_fit(desired_bpm, lines_per_beat, ticks_per_second);
    }
}

static void order_ticks_per_second(int *ticks_per_second_array)
{
    // This function will produce an array of numbers ordered like: 50,51,49,52,48,53,47...
    int delta = 0;
    for (int i = 0; i < TICKS_PER_SECOND_RANGE; i++)
    {
        ticks_per_second_array[i] = IDEAL_TICKS_PER_SECOND + delta;
        delta *= -1;
        if (i % 2 == 0) delta += 1;
    }
}

static tempo_t find_best_fit(const int desired_bpm, const int lines_per_beat, const int *ordered_ticks_per_second)
{
    float best_fit = 256.0f;
    tempo_t candidate = {0};
    for (int i = 0; i < TICKS_PER_SECOND_RANGE; i++)
    {
        const int ticks_per_second = ordered_ticks_per_second[i];
        for (int ticks_per_event = 1; ticks_per_event <= 255; ticks_per_event++)
        {
            const float epsilon = 0.0001f;
            const float events_per_second = (float) ticks_per_second / (float) ticks_per_event;
            const float actual_bpm = 60.0f * events_per_second / lines_per_beat;
            const float error = fabsf((float) desired_bpm - actual_bpm);
            if (error < epsilon)
            {
                return (tempo_t) {
                    .actual_bpm = actual_bpm,
                    .ticks_per_second = ticks_per_second,
                    .ticks_per_event = ticks_per_event,
                };
            }
            if (error < best_fit)
            {
                candidate.actual_bpm = actual_bpm;
                candidate.ticks_per_second = ticks_per_second;
                candidate.ticks_per_event = ticks_per_event;
                best_fit = error;
            }
        }
    }
    return candidate;
}