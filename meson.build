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

src = files(
  'src/main.c',
  'src/common/lstring.c'
)

executable('Morgunov',
  src,
  c_args: supported_flags,
  dependencies : ws2_32_dep
)
