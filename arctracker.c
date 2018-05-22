#include "arctracker.h"
#include "configuration.h"
#include "read_mod.h"
#include "play_mod.h"
#include "oss.h"
#include "alsa.h"

audio_api_t initialise_audio_api();

int main(int argc, char *argv[])
{
    read_configuration(argc, argv);
    module_t module = read_file();
    audio_api_t audio_api = initialise_audio_api();
    play_module(&module, audio_api);
    audio_api.finish();
    exit(EXIT_SUCCESS);
}

audio_api_t initialise_audio_api()
{
    switch (configuration().api)
    {
        case OSS:
            return initialise_oss();
        case ALSA:
            return initialise_alsa();
    }
}
