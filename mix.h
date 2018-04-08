void allocate_audio_buffer(int no_of_frames);

unsigned char *mix(long *channel_buffer, long channels_to_mix, int frames_to_mix);