#pragma once

#include <filesystem>
#include <string>
#include <vips/vips8>

namespace fs = std::filesystem;

enum DoublePageSpreadActions { ROTATE, SPLIT, BOTH, NONE };
enum RotationDirection { CLOCKWISE, COUNTERCLOCKWISE };

struct PageTask {
    fs::path source_file;
    fs::path output_dir;
    std::string output_base_name;
    std::string path_in_archive;
    std::string image_format;
    double dither;
    double quality;
    int page_number = -1;
#if defined(PDFIUM_ENABLED)
    int pdf_pixel_density;
#endif
    int page_width;
    int page_height;
    int bit_depth;
    int compression_effort;
    DoublePageSpreadActions double_page_spread_action;
    RotationDirection rotation_direction;
    VipsKernel page_resampler;
    bool convert_pages_to_greyscale;
    bool remove_spine;
    bool stretch_page_contrast;
    bool linear_light_resampling;
    bool scale_pages;
    bool quantize_pages;
    bool is_lossy;
    bool quality_type_is_distance;
};
