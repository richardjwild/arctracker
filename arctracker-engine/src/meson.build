main_sources = files(
    'arguments.c',
    'arctracker-console.c',
    'console_ui.c',
)

subdir('audio')
subdir('audio_api')
subdir('audio_file')
subdir('editor')
subdir('loader')
subdir('io')
subdir('memory')
subdir('midi')
subdir('pcm')
subdir('player')
subdir('lib')
subdir('ui')

libarctracker_sources = []
libarctracker_sources += lib_sources
libarctracker_sources += audio_sources
libarctracker_sources += audio_api_sources
libarctracker_sources += audio_file_sources
libarctracker_sources += loader_sources
libarctracker_sources += editor_sources
libarctracker_sources += io_sources
libarctracker_sources += memory_sources
libarctracker_sources += midi_sources
libarctracker_sources += pcm_sources
libarctracker_sources += player_sources
libarctracker_sources += ui_sources

arctracker_console_sources = []
arctracker_console_sources += main_sources
arctracker_console_sources += ui_sources
