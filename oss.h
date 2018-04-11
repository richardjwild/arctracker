#include "audio_api.h"

static int audio_handle;
static int audio_buffer_size_bytes;
static audio_api_t oss_audio_api;

audio_api_t initialise_oss(long p_sample_rate, int audio_buffer_frames);