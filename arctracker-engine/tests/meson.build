gtest_dep = dependency('gtest', fallback: ['gtest', 'gtest_dep'])
gtest_main_dep = dependency('gtest_main', fallback: ['gtest', 'gtest_main_dep'])

player_tests = executable(
    'player_tests',
    'PlayerTest.cpp',
    dependencies: [gtest_dep, gtest_main_dep],
    include_directories: project_includes,
    link_with: libarctracker,
)

test('player', player_tests)