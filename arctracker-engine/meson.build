project('arctracker', ['c', 'cpp'], version: '0.4.2')

cmake = import('cmake')

portaudio_options = cmake.subproject_options()
portaudio_options.add_cmake_defines({
    'PA_BUILD_STATIC': true,
    'PA_BUILD_SHARED': false,
    'PA_BUILD_TESTS': false,
    'PA_BUILD_EXAMPLES': false,
    'CMAKE_POLICY_VERSION_MINIMUM': '3.5',
})
portaudio_project = cmake.subproject(
    'portaudio-19.7.0',
    options: portaudio_options,
)
portaudio = portaudio_project.dependency('portaudio_static')

rtmidi_options = cmake.subproject_options()
rtmidi_options.add_cmake_defines({
    'RTMIDI_BUILD_STATIC_LIBS': true,
    'RTMIDI_BUILD_SHARED_LIBS': false,
    'RTMIDI_BUILD_TESTING': false,
})
rtmidi_project = cmake.subproject(
    'rtmidi-6.0.0',
    options: rtmidi_options,
)
rtmidi = rtmidi_project.dependency('rtmidi')

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-Werror=implicit-function-declaration',
  language: 'c'
)

if get_option('asan')
  add_project_arguments('-fsanitize=address', language: ['c', 'cpp'])
  add_project_link_arguments('-fsanitize=address', language: ['c', 'cpp'])
endif

subdir('src')
project_includes = include_directories('src')

libarctracker = static_library('arctracker',
  libarctracker_sources,
  dependencies: [portaudio, rtmidi],
  include_directories: project_includes,
)

executable('arctracker-console',
  arctracker_console_sources,
  link_with: libarctracker,
  dependencies: [portaudio, rtmidi],
  include_directories: project_includes,
  install: true
)
