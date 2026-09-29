#include "audio_channel.h"

void audio_channel_silence(audio_channel_t *channel)
{
    channel->audio_generator = null_audio_generator();
}

void audio_channel_set_stereo(audio_channel_t *channel, const uint8_t stereo)
{
    channel->audio_spatialiser.set_amount(&channel->audio_spatialiser.state, stereo);
}

uint8_t audio_channel_get_stereo(audio_channel_t *channel)
{
    return channel->audio_spatialiser.get_amount(&channel->audio_spatialiser.state);
}
