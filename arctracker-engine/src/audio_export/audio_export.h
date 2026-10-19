#ifndef ARCTRACKER_API_WAV_H
#define ARCTRACKER_API_WAV_H

#include <stdbool.h>

#define MAX_FILE_PATH_LENGTH 4096

typedef struct {
    char output_filename[MAX_FILE_PATH_LENGTH];
    unsigned long frames_written;
    unsigned long data_capacity;
    float *audio_buffer;
    unsigned long audio_buffer_size;
    int16_t *audio_data;
} wav_export_t;

bool export_init(const char *output_filename, wav_export_t *wav_export);

bool export_collect_audio(wav_export_t *wav_export, unsigned long frames_in_buffer);

bool export_finish(wav_export_t *wav_export);

#endif //ARCTRACKER_API_WAV_H
