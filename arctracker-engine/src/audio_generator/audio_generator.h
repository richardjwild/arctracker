#ifndef ARCTRACKER_ENGINE_AUDIO_GENERATOR_H
#define ARCTRACKER_ENGINE_AUDIO_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "player/lfo.h"

typedef struct sampler_state sampler_state_t;

typedef union {
    sampler_state_t *sampler;
} audio_generator_state_t;

/**************************************************************************************************
 * An audio generator is required to provide valid function pointers for all of these functions,  *
 * although the only one that is required to do anything is generate_audio. This one is called    *
 * by the audio subsystem when it requires the generator to place the requested number of sample  *
 * frames in the channel buffer.                                                                  *
 *                                                                                                *
 * The tick function is called on every non-zero tick, and gives the generator its opportunity to *
 * update its internal state if it needs to. This might be so that it can implement vibrato,      *
 * tremolo, pitch and volume slides, etc. But the generator is free to do nothing here if it does *
 * not need to do anything.                                                                       *
 *                                                                                                *
 * The remaining functions are called in response to tracker commands, and are there to control   *
 * generator effects such as the aforementioned vibrato, tremolo, pitch and volume slides, etc.   *
 * The generator may or may not implement these effects, and if not, it is once again free to do  *
 * nothing when any of these functions are called.                                                *
 *************************************************************************************************/

typedef struct {
    audio_generator_state_t state;
    bool (*generate_audio)(audio_generator_state_t *state, float *channel_buffer, int frames_requested);
    void (*tick)(audio_generator_state_t *state, int ticks, int ticks_per_event);
    void (*set_volume)(audio_generator_state_t *state, uint8_t volume);
    void (*volume_slide_on)(audio_generator_state_t *state, int slide_rate, bool fine);
    void (*volume_slide_off)(audio_generator_state_t *state);
    void (*pitch_slide_on)(audio_generator_state_t *state, int slide_rate, bool fine);
    void (*pitch_slide_off)(audio_generator_state_t *state);
    void (*set_tone_portamento_target)(audio_generator_state_t *state, int target_note);
    void (*tone_portamento_on)(audio_generator_state_t *state, int slide_rate);
    void (*tone_portamento_off)(audio_generator_state_t *state);
    void (*vibrato_on)(audio_generator_state_t *state, int rate, int depth, pt_waveform_t waveform, bool retrigger);
    void (*vibrato_off)(audio_generator_state_t *state);
    void (*tremolo_on)(audio_generator_state_t *state, int rate, int depth, pt_waveform_t waveform, bool retrigger);
    void (*tremolo_off)(audio_generator_state_t *state);
    void (*arpeggio_on)(audio_generator_state_t *state, int root_note, int interval_1, int interval_2, int speed);
    void (*arpeggio_off)(audio_generator_state_t *state);
    void (*set_glissando)(audio_generator_state_t *state, bool enabled);
    void (*silence_after_delay)(audio_generator_state_t *state, int ticks);
    void (*retrigger)(audio_generator_state_t *state, int ticks);
} audio_generator_t;

#endif //ARCTRACKER_ENGINE_AUDIO_GENERATOR_H
