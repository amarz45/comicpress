#pragma once

#include <filesystem>
#include <string>
#include <vips/vips8>

namespace fs = std::filesystem;

enum DoublePageSpreadActions { ROTATE, SPLIT, BOTH, NONE };

struct PageTask {
    fs::path source_file;
    fs::path output_dir;
    std::string output_base_name;
    int page_number = -1;
    std::string path_in_archive;
    int pdf_pixel_density;
    DoublePageSpreadActions double_page_spread_action;
    bool stretch_page_contrast;
    bool scale_pages;
    int page_width;
    int page_height;
    VipsKernel page_resampler;
    bool quantize_pages;
    int bit_depth;
    float dither;
    std::string image_format;
    bool is_lossy;
    bool quality_type_is_distance;
    int compression_effort;
};
