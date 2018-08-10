#include "../playroutine/play_mod.h"

const static int PITCH_QUANTA = 2047;
const static double PHASE_INCREMENT_CONVERSION = 3273808.59375;

static double *phase_increments;
static float *resample_buffer;
static size_t resample_buffer_bytes;

void calculate_phase_increments(long p_sample_rate);

void allocate_resample_buffer(int no_of_frames);

float *resample(voice_t* voice, long frames_to_write);
