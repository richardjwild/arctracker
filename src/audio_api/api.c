#include <config.h>

#if HAVE_LIBASOUND

#include "api.h"
#include "api_alsa.h"

audio_api_t initialise_audio_api()
{
    return initialise_alsa();
}

#elif HAVE_LIBPORTAUDIO

#include "api_portaudio.h"

audio_api_t initialise_audio_api()
{
    return initialise_portaudio();
}

#elif HAVE_SYS_SOUNDCARD_H

#include "api_oss.h"

audio_api_t initialise_audio_api()
{
    return initialise_oss();
}

#endif
