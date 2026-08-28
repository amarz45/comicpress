#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vips/vips8>

namespace fs = std::filesystem;

enum class DoublePageSpreadActions : std::uint8_t { ROTATE, SPLIT, BOTH, NONE };
enum class RotationDirection : std::uint8_t { CLOCKWISE, COUNTERCLOCKWISE };
enum class ImageFormat : std::uint8_t { AVIF, JPEG, JPEG_XL, PNG, WEBP };
enum class CompressionType : std::uint8_t { LOSSLESS, LOSSY };
enum class QualityType : std::uint8_t { QUALITY, DISTANCE };

struct PageTask {
    fs::path source_file;
    fs::path output_dir;
    std::string output_base_name;
    std::string path_in_archive;
    ImageFormat image_format;
    double dither;
    double quality;
    int page_number = -1;
#if defined(PDF_ENABLED)
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
    CompressionType compression_type;
    QualityType quality_type;
};
