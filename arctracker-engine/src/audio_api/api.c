#include "api.h"
#include "api_portaudio.h"
#include "api_wav.h"

audio_api_t create_audio_api(bool bounce, char *output_filename)
{
    return bounce
            ? initialise_wav(output_filename)
            : create_portaudio_api();
}

audio_api_result_t audio_api_failure(const char *error_message)
{
    return (audio_api_result_t) {
        .success = false,
        .error_message = error_message
    };
}

