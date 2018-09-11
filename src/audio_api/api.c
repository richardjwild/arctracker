#include <config.h>
#include "api_wav.h"

#if HAVE_LIBASOUND

#include "api.h"
#include "api_alsa.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename ? initialise_wav(config.output_filename) : initialise_alsa();
}


#elif HAVE_LIBPORTAUDIO
#include "api_portaudio.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename ? initialise_wav(config.output_filename) : initialise_portaudio();
}

#elif HAVE_SYS_SOUNDCARD_H

#include "api_oss.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename ? initialise_wav(config.output_filename) : initialise_oss();
}

#endif
