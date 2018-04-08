static const int DBFS_0_POSITIVE = 32767;
static const int DBFS_0_NEGATIVE = -32768;

static __int16_t *audio_buffer;
static int audio_buffer_frames;

void allocate_audio_buffer(int no_of_frames);

__int16_t *mix(const long *channel_buffer, int channels_to_mix);