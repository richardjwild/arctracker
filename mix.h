void allocate_audio_buffer(int no_of_frames);

unsigned char *mix(const long *channel_buffer, const long channels_to_mix, const int frames_to_mix);