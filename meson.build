project('comicpress', 'cpp')

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
    moc_headers: ['src/window.h']
)

executable(
    'comicpress', 'src/main.cpp', 'src/window.cpp', 'src/processing.cpp',
    qt_processed_files,
    dependencies: [vips_dep, qt6_dep, pdfium_dep, libarchive_dep, zlib_dep],
    include_directories: pdfium_inc,
    install: true,
)
