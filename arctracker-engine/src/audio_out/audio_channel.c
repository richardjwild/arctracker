#include "audio_channel.h"

static const uint8_t PAN_CENTRE = 0x80;

void audio_channel_silence(audio_channel_t *channel)
{
    channel->audio_generator = null_audio_generator();
}

void audio_channel_set_stereo(audio_channel_t *channel, const uint8_t stereo)
{
    channel->panning = stereo == 0 ? PAN_CENTRE : stereo;
}
