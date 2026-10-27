conversion_lib = static_library(
    'conversion',
    'conversion.cpp',
    'output_formats.cpp',
    dependencies: [io_dep, qt6_core_dep, pdfium_dep],
)

conversion_dep = declare_dependency(
    link_with: conversion_lib,
    include_directories: include_directories('include'),
    dependencies: [io_dep, qt6_core_dep, pdfium_dep],
)
