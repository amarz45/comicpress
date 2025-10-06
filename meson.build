project('comicpress', 'cpp',
    default_options : [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20'
    ]
)

if get_option('buildtype') == 'release'
    add_project_arguments('-flto', language: 'cpp')
    add_project_link_arguments('-flto', language: 'cpp')
endif

cpp = meson.get_compiler('cpp')

vips_dep = dependency('vips-cpp')
qt6_dep = dependency('qt6', modules: ['Widgets'])
libarchive_dep = dependency('libarchive')
zlib_dep = dependency('zlib')

pdfium_dep = dependency('pdfium', required: false)
pdfium_inc = include_directories('/usr/include')

if not pdfium_dep.found()
  pdfium_dep = cpp.find_library('pdfium')
endif

qt6 = import('qt6')

qt_processed_files = qt6.preprocess(
    moc_headers: ['src/gui/include/window.hpp']
)

# Worker executable, this does the actual image processing.
executable(
    'comicpress-worker', 'src/worker/main.cpp', 'src/worker/processing.cpp',
    dependencies: [vips_dep, pdfium_dep, libarchive_dep, zlib_dep],
    include_directories: pdfium_inc,
    install: true,
)

executable(
    'comicpress', 'src/gui/main.cpp', 'src/gui/window.cpp',
    'src/gui/signal.cpp', 'src/gui/time.cpp', 'src/gui/options.cpp',
    'src/gui/window_util.cpp',
    qt_processed_files,
    dependencies: [qt6_dep, pdfium_dep, libarchive_dep, vips_dep],
    include_directories: pdfium_inc,
    install: true,
)
