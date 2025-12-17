project(
    'comicpress',
    'cpp',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++26'],
)

if get_option('buildtype') == 'release'
    add_project_arguments('-flto', language: 'cpp')
    add_project_link_arguments('-flto', language: 'cpp')
endif

cpp = meson.get_compiler('cpp')

vips_dep = dependency('vips-cpp')
qt6_dep = dependency('qt6', modules: ['Widgets'])
libarchive_dep = dependency('libarchive')

pdfium_opt = get_option('pdfium')
pdfium_dep = dependency('', required: false)

if pdfium_opt.allowed()
    pdfium_dep = dependency('pdfium', required: false)
    if not pdfium_dep.found()
        pdfium_dep = cpp.find_library('pdfium', required: false)
    endif
endif

if pdfium_dep.found()
    add_project_arguments('-DPDFIUM_ENABLED', language: 'cpp')
elif pdfium_opt.enabled()
    error(
        'PDFium support was explicitly enabled but the library was not found.',
    )
endif

qt6 = import('qt6')
qt_processed_files = qt6.preprocess(moc_headers: ['src/gui/include/window.hpp', 'src/gui/include/options.hpp'])

# Worker executable, this does the actual image processing.
executable(
    'comicpress-worker',
    'src/worker/main.cpp',
    'src/worker/processing.cpp',
    dependencies: [vips_dep, pdfium_dep, libarchive_dep],
    install: true,
)

executable(
    'comicpress',
    'src/gui/main.cpp',
    'src/gui/window.cpp',
    'src/gui/signal.cpp',
    'src/gui/time.cpp',
    'src/gui/options.cpp',
    'src/gui/window_util.cpp',
    qt_processed_files,
    dependencies: [qt6_dep, pdfium_dep, libarchive_dep, vips_dep],
    install: true,
)
