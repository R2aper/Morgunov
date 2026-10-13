project('Morgunov', 'c',
  default_options : [
    'warning_level=3',
    'c_std=c11',
  ]
)

cc = meson.get_compiler('c')

# link ws2_32 for windows (MSVC will resolve it via pragma comment, but MinGW will not)
ws2_32_dep = cc.find_library('ws2_32', required : host_machine.system() == 'windows')
extra_flags = [
  '-Wpedantic', '-Wshadow', '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wconversion', '-Werror'
]

supported_flags = cc.get_supported_arguments(extra_flags)

core_src = [
  'src/common/lstring.c',
  'src/common/event.c',
  'src/network/connectionHandler.c',
]
main_src = core_src + ['src/main.c']
test_src = [
  'test/main.c',
  'test/morgutest.c',
  'test/unitDynarray.c',
  'test/unitEvent.c'
] + core_src

executable('Morgunov',
  main_src,
  c_args: supported_flags,
  dependencies : ws2_32_dep
)

test_exe = executable('MorgunovTest', 
test_src,
dependencies: ws2_32_dep
)

test('Morgunov Tests', test_exe, verbose: true)
