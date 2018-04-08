const static int PITCH_QUANTA = 2047;
const static double PHASE_INCREMENT_CONVERSION = 3273808.59375;

static double *phase_increments;
static unsigned char *resample_buffer;

void calculate_phase_increments(const long p_sample_rate);

void allocate_resample_buffer();

unsigned char* resample(channel_info* voice, const long frames_to_write);
