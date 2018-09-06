#include "arctracker.h"
#include <audio_api/alsa.h>
#include <audio_api/oss.h>
#include <formats/modfile_formats.h>
#include <io/configuration.h>
#include <io/read_mod.h>
#include <playroutine/play_mod.h>

audio_api_t initialise_audio_api();

int main(int argc, char *argv[])
{
    read_configuration(argc, argv);
    module_t module = read_file(formats(), num_formats());
    if (!configuration().info)
    {
        audio_api_t audio_api = initialise_audio_api();
        play_module(&module, audio_api);
        audio_api.finish();
    }
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
