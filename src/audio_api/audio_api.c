#include "audio_api.h"
#include "alsa.h"
#include "oss.h"
#include "port_audio.h"

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