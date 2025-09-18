project('comicpress', 'cpp')

vips_dep = dependency('vips-cpp')
qt6_dep = dependency('qt6', modules: ['Widgets'])
mupdf_dep = dependency('mupdf')
libarchive_dep = dependency('libarchive')

qt6 = import('qt6')

qt_processed_files = qt6.preprocess(
    moc_headers: ['src/window.h', 'src/worker.h']
)

executable(
    'comicpress', 'src/main.cpp', 'src/window.cpp', 'src/processing.cpp',
    'src/worker.cpp', qt_processed_files,
    dependencies: [vips_dep, qt6_dep, mupdf_dep, libarchive_dep],
    install: true,
)
