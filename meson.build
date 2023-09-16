project(
    'escapeline',
    'cpp',
    version: '0.1.0',
    default_options: ['cpp_std=c++20'],
)

srcs = ['main.cpp']
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

executable(
    'el',
    srcs,
    install: true,
)
