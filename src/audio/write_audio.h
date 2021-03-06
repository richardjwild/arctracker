#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

#include <playroutine/play_mod.h>

void initialise_audio(audio_api_t audio_output, int channels);

void write_audio_data(voice_t *voices);

void send_remaining_audio();

#endif // ARCTRACKER_WRITE_AUDIO_H