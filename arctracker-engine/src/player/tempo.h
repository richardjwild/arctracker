#ifndef ARCTRACKER_ENGINE_TEMPO_H
#define ARCTRACKER_ENGINE_TEMPO_H

typedef struct {
    float actual_bpm;
    int ticks_per_event;
    int ticks_per_second;
} tempo_t;

void calculate_tempo(tempo_t *tempo_array, int lines_per_beat, int max_bpm);

#endif //ARCTRACKER_ENGINE_TEMPO_H
