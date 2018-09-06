#include "arctracker.h"
#include <audio_api/audio_api.h>
#include <formats/modfile_formats.h>
#include <io/configuration.h>
#include <io/read_mod.h>
#include <playroutine/play_mod.h>

int main(int argc, char *argv[])
{
    read_configuration(argc, argv);
    args_t config = configuration();
    module_t module = read_file(formats(), num_formats());
    if (!config.info)
    {
        audio_api_t audio_api = initialise_audio_api(config.api);
        play_module(&module, audio_api);
        audio_api.finish();
    }
    exit(EXIT_SUCCESS);
}
