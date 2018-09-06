cmake_minimum_required(VERSION 3.12)
project(arctracker C)

set(CMAKE_C_STANDARD 11)

include_directories(src)
include_directories(src/audio)
include_directories(src/audio_api)
include_directories(src/chrono)
include_directories(src/formats)
include_directories(src/io)
include_directories(src/memory)
include_directories(src/pcm)
include_directories(src/playroutine)

add_executable(arctracker
        src/audio/audio_api.h
        src/audio/gain.c
        src/audio/gain.h
        src/audio/mix.c
        src/audio/mix.h
        src/audio/period.c
        src/audio/period.h
        src/audio/resample.c
        src/audio/resample.h
        src/audio/write_audio.c
        src/audio/write_audio.h
        src/audio_api/alsa.c
        src/audio_api/alsa.h
        src/audio_api/oss.c
        src/audio_api/oss.h
        src/chrono/clock.c
        src/chrono/clock.h
        src/formats/desktop_tracker_module.c
        src/formats/desktop_tracker_module.h
        src/formats/tracker_module.c
        src/formats/tracker_module.h
        src/io/configuration.c
        src/io/configuration.h
        src/io/console.c
        src/io/console.h
        src/io/error.c
        src/io/error.h
        src/io/read_mod.c
        src/io/read_mod.h
        src/memory/bits.h
        src/memory/heap.c
        src/memory/heap.h
        src/pcm/mu_law.c
        src/pcm/mu_law.h
        src/playroutine/effects.c
        src/playroutine/effects.h
        src/playroutine/play_mod.c
        src/playroutine/play_mod.h
        src/playroutine/sequence.c
        src/playroutine/sequence.h
        src/arctracker.c
        src/arctracker.h
        src/config.h
        src/formats/modfile_formats.c
        src/formats/modfile_formats.h
        src/audio_api/audio_api.c
        src/audio_api/audio_api.h)

target_link_libraries(
        arctracker
        asound)
