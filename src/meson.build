# Common code shared by the GUI and the worker.
common_lib = static_library(
    'common',
    'archive.cpp',
    'task.cpp',
    dependencies: [vips_dep, libarchive_dep],
)

common_dep = declare_dependency(
    link_with: common_lib,
    include_directories: include_directories('include'),
    dependencies: [vips_dep, libarchive_dep],
)

subdir('worker')
subdir('gui')
