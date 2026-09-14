#include "audio_generator/audio_generator.h"
#include <string.h>

static bool generate_audio(audio_generator_state_t *, float *, int);
static void tick(audio_generator_state_t *, int, int);
static void set_volume(audio_generator_state_t *state, uint8_t);
static void volume_slide_on(audio_generator_state_t *state, int, bool);
static void volume_slide_off(audio_generator_state_t *state);
static void pitch_slide_on(audio_generator_state_t *state, int, bool);
static void pitch_slide_off(audio_generator_state_t *state);
static void set_tone_portamento_target(audio_generator_state_t *, int);
static void tone_portamento_on(audio_generator_state_t *, int);
static void tone_portamento_off(audio_generator_state_t *);
static void vibrato_on(audio_generator_state_t *, int, int, pt_waveform_t, bool);
static void vibrato_off(audio_generator_state_t *);
static void tremolo_on(audio_generator_state_t *, int, int, pt_waveform_t, bool);
static void tremolo_off(audio_generator_state_t *);
static void arpeggio_on(audio_generator_state_t *, int, int, int, int);
static void arpeggio_off(audio_generator_state_t *);
static void set_glissando(audio_generator_state_t *, bool);
static void silence_after_delay(audio_generator_state_t *, int);
static void retrigger(audio_generator_state_t *, int);
static void advance_phase(audio_generator_state_t *, int);

audio_generator_t null_audio_generator(void)
{
    return (audio_generator_t) {
        .state = (audio_generator_state_t) {0},
        .generate_audio = generate_audio,
        .tick = tick,
        .set_volume = set_volume,
        .volume_slide_on = volume_slide_on,
        .volume_slide_off = volume_slide_off,
        .pitch_slide_on = pitch_slide_on,
        .pitch_slide_off = pitch_slide_off,
        .set_tone_portamento_target = set_tone_portamento_target,
        .tone_portamento_on = tone_portamento_on,
        .tone_portamento_off = tone_portamento_off,
        .vibrato_on = vibrato_on,
        .vibrato_off = vibrato_off,
        .tremolo_on = tremolo_on,
        .tremolo_off = tremolo_off,
        .arpeggio_on = arpeggio_on,
        .arpeggio_off = arpeggio_off,
        .set_glissando = set_glissando,
        .silence_after_delay = silence_after_delay,
        .retrigger = retrigger,
        .advance_phase = advance_phase,
    };
}

static bool generate_audio(audio_generator_state_t *state, float *channel_buffer, const int no_frames)
{
    (void) state;
    memset(channel_buffer, 0, sizeof(float) * no_frames);
    return true;
}

static void tick(audio_generator_state_t *state, int ticks, int ticks_per_event)
{
    (void) state;
    (void) ticks;
    (void) ticks_per_event;
}

static void set_volume(audio_generator_state_t *state, uint8_t volume)
{
    (void) state;
    (void) volume;
}

static void volume_slide_on(audio_generator_state_t *state, int slide_rate, bool fine)
{
    (void) state;
    (void) slide_rate;
    (void) fine;
}

static void volume_slide_off(audio_generator_state_t *state)
{
    (void) state;
}

static void pitch_slide_on(audio_generator_state_t *state, int slide_rate, bool fine)
{
    (void) state;
    (void) slide_rate;
    (void) fine;
}

static void pitch_slide_off(audio_generator_state_t *state)
{
    (void) state;
}

static void set_tone_portamento_target(audio_generator_state_t *state, int target_note)
{
    (void) state;
    (void) target_note;
}

static void tone_portamento_on(audio_generator_state_t *state, int slide_rate)
{
    (void) state;
    (void) slide_rate;
}

static void tone_portamento_off(audio_generator_state_t *state)
{
    (void) state;
}

static void vibrato_on(audio_generator_state_t *state, int rate, int depth, pt_waveform_t waveform, bool retrigger)
{
    (void) state;
    (void) rate;
    (void) depth;
    (void) waveform;
    (void) retrigger;
}

static void vibrato_off(audio_generator_state_t *state)
{
    (void) state;
}

static void tremolo_on(audio_generator_state_t *state, int rate, int depth, pt_waveform_t waveform, bool retrigger)
{
    (void) state;
    (void) rate;
    (void) depth;
    (void) waveform;
    (void) retrigger;
}

static void tremolo_off(audio_generator_state_t *state)
{
    (void) state;
}

static void arpeggio_on(audio_generator_state_t *state, int bottom_note, int interval_1, int interval_2, int speed)
{
    (void) state;
    (void) bottom_note,
    (void) interval_1;
    (void) interval_2;
    (void) speed;
}

static void arpeggio_off(audio_generator_state_t *state)
{
    (void) state;
}

static void set_glissando(audio_generator_state_t *state, bool glissando_on)
{
    (void) state;
    (void) glissando_on;

}
static void silence_after_delay(audio_generator_state_t *state, int ticks)
{
    (void) state;
    (void) ticks;
}

static void retrigger(audio_generator_state_t *state, int ticks)
{
    (void) state;
    (void) ticks;
}

static void advance_phase(audio_generator_state_t *state, int frames)
{
    (void) state;
    (void) frames;
}
