#include "audio_channel.h"

void silence_channel(audio_channel_t *channel)
{
    channel->audio_generator = null_audio_generator();
}