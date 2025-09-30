project('comicpress', 'cpp')

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-Werror',
  language: 'cpp'
)

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
    moc_headers: ['src/include/window.hpp']
)

# Sources and dependencies for the processing logic, shared by the worker.
processing_src = ['src/processing.cpp']
processing_deps = [vips_dep, pdfium_dep, libarchive_dep, zlib_dep]

# Worker executable, this does the actual image processing.
executable(
    'comicpress-worker', 'src/worker.cpp', processing_src,
    dependencies: processing_deps,
    include_directories: pdfium_inc,
    install: true,
)

executable(
    'comicpress', 'src/main.cpp', 'src/window.cpp',
    qt_processed_files,
    dependencies: [qt6_dep, pdfium_dep, libarchive_dep, vips_dep],
    include_directories: pdfium_inc,
    install: true,
)
