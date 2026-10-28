main_sources = files(
    'arguments.c',
    'arctracker-console.c',
    'console_ui.c',
)

subdir('audio_export')
subdir('audio_file')
subdir('audio_generator')
subdir('audio_spatialiser')
subdir('audio_out')
subdir('concurrency')
subdir('editor')
subdir('fraction')
subdir('io')
subdir('lib')
subdir('loader')
subdir('memory')
subdir('midi')
subdir('player')
subdir('portaudio')
subdir('ui')
subdir('vidc')

libarctracker_sources = []
libarctracker_sources += lib_sources
libarctracker_sources += audio_export_sources
libarctracker_sources += audio_file_sources
libarctracker_sources += audio_generator_sources
libarctracker_sources += audio_spatialiser_sources
libarctracker_sources += audio_out_sources
libarctracker_sources += concurrency_sources
libarctracker_sources += editor_sources
libarctracker_sources += fraction_sources
libarctracker_sources += io_sources
libarctracker_sources += loader_sources
libarctracker_sources += memory_sources
libarctracker_sources += midi_sources
libarctracker_sources += player_sources
libarctracker_sources += portaudio_sources
libarctracker_sources += ui_sources
libarctracker_sources += vidc_sources

arctracker_console_sources = []
arctracker_console_sources += main_sources
arctracker_console_sources += ui_sources
