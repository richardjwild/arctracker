#include <config.h>
#include "api.h"

#if HAVE_LIBASOUND

#include "api_alsa.h"
#include "api_wav.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename
            ? initialise_wav(config.output_filename)
            : initialise_alsa();
}

#elif HAVE_LIBPORTAUDIO

#include "api_portaudio.h"
#include "api_wav.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename
            ? initialise_wav(config.output_filename)
            : initialise_portaudio();
}

#elif HAVE_SYS_SOUNDCARD_H

#include "api_oss.h"
#include "api_wav.h"

audio_api_t initialise_audio_api(args_t config)
{
    return config.output_filename
            ? initialise_wav(config.output_filename)
            : initialise_oss();
}

#else

#include "api_wav.h"
#include <io/error.h>

audio_api_t initialise_audio_api(args_t config)
{
    if (!config.output_filename)
    {
        error("No audio system support available. WAV file only.");
    }
    return initialise_wav(config.output_filename);
}

#endif
