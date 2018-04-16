#include "play_mod.h"

#ifndef ARCTRACKER_WRITE_AUDIO_H
#define ARCTRACKER_WRITE_AUDIO_H

void initialise_audio(audio_api_t audio_output, long channels);

void write_audio_data(channel_info *voices, long frames_requested);

void send_remaining_audio();

#endif // ARCTRACKER_WRITE_AUDIO_H