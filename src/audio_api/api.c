#include "api.h"
#include "api_alsa.h"
#include "api_oss.h"
#include "api_portaudio.h"

audio_api_t initialise_audio_api(output_api api)
{
    switch (api)
    {
        case OSS:
            return initialise_oss();
        case ALSA:
            return initialise_alsa();
        case PORTAUDIO:
            return initialise_portaudio();
    }
}