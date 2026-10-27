# Common code shared by the GUI and the worker.
io_lib = static_library(
    'io',
    'archive.cpp',
    'task.cpp',
    dependencies: [vips_dep, libarchive_dep],
)

io_dep = declare_dependency(
    link_with: io_lib,
    include_directories: include_directories('include'),
    dependencies: [vips_dep, libarchive_dep],
)
