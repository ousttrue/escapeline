project(
    'escapeline',
    'cpp',
    version: '0.1.0',
    default_options: ['cpp_std=c++20'],
)

srcs = ['main.cpp', 'el.cpp',]
if host_machine.system() == 'windows'
    srcs += [
        'el_term_windows.cpp',
        'el_pty_windows.cpp',
    ]
else
    srcs += [
        'el_term_posix.cpp',
        'el_pty_posix.cpp',
    ]
endif

lua_dep = dependency('lua', default_options: ['default_library=static'])
uv_dep = dependency('libuv', default_options: ['default_library=static'])
cxxopts_dep = dependency('cxxopts')

executable(
    'el',
    srcs,
    install: true,
    dependencies: [lua_dep, uv_dep, cxxopts_dep],
)
