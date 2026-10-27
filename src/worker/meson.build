worker_lib = static_library(
    'worker',
    'processing.cpp',
    'worker.cpp',
    dependencies: [io_dep, pdfium_dep],
)

worker_dep = declare_dependency(
    link_with: worker_lib,
    include_directories: include_directories('include'),
    dependencies: [io_dep, pdfium_dep],
)
