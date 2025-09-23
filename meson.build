#project('comicpress', 'cpp')

#cpp = meson.get_compiler('cpp')

#vips_dep = dependency('vips-cpp')
#qt6_dep = dependency('qt6', modules: ['Widgets'])
##mupdf_dep = dependency('mupdf')
#libarchive_dep = dependency('libarchive')
#mupdf_lib = cpp.find_library('mupdf')
#mupdf_inc = include_directories('/usr/include')

#qt6 = import('qt6')

#qt_processed_files = qt6.preprocess(
#    moc_headers: ['src/window.h', 'src/worker.h']
#)

#executable(
#    'comicpress', 'src/main.cpp', 'src/window.cpp', 'src/processing.cpp',
#    'src/worker.cpp', qt_processed_files,
#    dependencies: [vips_dep, qt6_dep, mupdf_lib, libarchive_dep],
#    include_directories: mupdf_inc,
#    install: true,
#)

project('comicpress', 'cpp')

cpp = meson.get_compiler('cpp')

vips_dep = dependency('vips-cpp')
qt6_dep = dependency('qt6', modules: ['Widgets'])
libarchive_dep = dependency('libarchive')
zlib_dep = dependency('zlib')

# Find the PDFium library
pdfium_lib = cpp.find_library('pdfium')

qt6 = import('qt6')

qt_processed_files = qt6.preprocess(
    moc_headers: ['src/window.h']
)

executable(
    'comicpress', 'src/main.cpp', 'src/window.cpp', 'src/processing.cpp',
    qt_processed_files,
    dependencies: [vips_dep, qt6_dep, pdfium_lib, libarchive_dep, zlib_dep],
    install: true,
)
