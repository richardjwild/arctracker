project('arctracker', 'c', version: '0.4.2')

portaudio = dependency('portaudio-2.0')

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-Werror=implicit-function-declaration',
  language: 'c'
)

if get_option('asan')
  add_project_arguments('-fsanitize=address', language: 'c')
  add_project_link_arguments('-fsanitize=address', language: 'c')
endif

subdir('src')
project_includes = include_directories('src')

libarctracker = static_library('arctracker',
  libarctracker_sources,
  dependencies: portaudio,
  include_directories: project_includes,
)

executable('arctracker-console',
  arctracker_console_sources,
  link_with: libarctracker,
  dependencies: portaudio,
  include_directories: project_includes,
  install: true
)
