long* calculate_phase_increments(const long p_sample_rate);

void allocate_resample_buffer();

unsigned char* resample(channel_info* voice, const long frames_to_write);
