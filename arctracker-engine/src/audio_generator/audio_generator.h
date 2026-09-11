#ifndef ARCTRACKER_ENGINE_AUDIO_GENERATOR_H
#define ARCTRACKER_ENGINE_AUDIO_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "player/lfo.h"

typedef struct sampler_state sampler_state_t;

typedef union {
    sampler_state_t *sampler;
} audio_generator_state_t;

typedef struct {
    audio_generator_state_t state;
    bool (*generate_audio)(audio_generator_state_t *, float *, int);
    void (*tick)(audio_generator_state_t *, int, int);
    void (*set_volume)(audio_generator_state_t *, uint8_t);
    void (*volume_slide_on)(audio_generator_state_t *, int, bool);
    void (*volume_slide_off)(audio_generator_state_t *);
    void (*pitch_slide_on)(audio_generator_state_t *, int, bool);
    void (*pitch_slide_off)(audio_generator_state_t *);
    void (*set_tone_portamento_target)(audio_generator_state_t *, int);
    void (*tone_portamento_on)(audio_generator_state_t *, int);
    void (*tone_portamento_off)(audio_generator_state_t *);
    void (*vibrato_on)(audio_generator_state_t *, int, int, pt_waveform_t, bool);
    void (*vibrato_off)(audio_generator_state_t *);
    void (*tremolo_on)(audio_generator_state_t *, int, int, pt_waveform_t, bool);
    void (*tremolo_off)(audio_generator_state_t *);
    void (*arpeggio_on)(audio_generator_state_t *, int, int, int, int);
    void (*arpeggio_off)(audio_generator_state_t *);
    void (*set_glissando)(audio_generator_state_t *, bool);
    void (*silence_after_delay)(audio_generator_state_t *, int);
    void (*retrigger)(audio_generator_state_t *, int);
} audio_generator_t;

#endif //ARCTRACKER_ENGINE_AUDIO_GENERATOR_H
