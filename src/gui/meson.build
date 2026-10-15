gui_moc_files = qt6.preprocess(
    moc_headers: ['include/window.hpp', 'include/options.hpp'],
)

gui_lib = static_library(
    'gui',
    'options.cpp',
    'output_formats.cpp',
    'settings.cpp',
    'signal.cpp',
    'time.cpp',
    'window.cpp',
    'window_util.cpp',
    gui_moc_files,
    dependencies: [common_dep, qt6_dep, pdfium_dep],
    cpp_pch: '../../pch/pch.hpp',
)

gui_dep = declare_dependency(
    link_with: gui_lib,
    include_directories: include_directories('include'),
    dependencies: [common_dep, qt6_dep, pdfium_dep],
)
