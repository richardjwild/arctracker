long* calculate_phase_increments(long p_sample_rate);

void allocate_resample_buffer();

unsigned char* resample(channel_info* voice, long frames_to_write);
