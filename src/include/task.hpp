#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vips/vips8>

namespace fs = std::filesystem;

enum class DoublePageSpreadActions : std::uint8_t { ROTATE, SPLIT, BOTH, NONE };
enum class ReadingDirection : std::uint8_t { LTR, RTL };
enum class RotationDirection : std::uint8_t { CLOCKWISE, COUNTERCLOCKWISE };
enum class ImageFormat : std::uint8_t { AVIF, JPEG, JPEG_XL, PNG, WEBP };
enum class CompressionType : std::uint8_t { LOSSLESS, LOSSY };
enum class QualityType : std::uint8_t { QUALITY, DISTANCE };

enum class BitDepth : std::uint8_t {
    ONE = 1,
    TWO = 2,
    FOUR = 4,
    EIGHT = 8,
    SIXTEEN = 16,
};

struct ProcessingSettings {
    ProcessingSettings() = default;
    ImageFormat image_format;
    double dither;
    double quality;
#if defined(PDF_ENABLED)
    int pdf_pixel_density;
#endif
    int page_width;
    int page_height;
    BitDepth bit_depth;
    int compression_effort;
    DoublePageSpreadActions double_page_spread_action;
    ReadingDirection reading_direction;
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
static_assert(std::is_trivially_copyable_v<ProcessingSettings>);

using ProcessingSettingsBytes
    = std::array<std::uint8_t, sizeof(ProcessingSettings)>;

struct PageTask {
    fs::path source_file;
    fs::path output_dir;
    std::string output_base_name;
    std::string path_in_archive;
    int page_number = -1;
    ProcessingSettings processing_settings{};
};

inline void for_each_task_field(auto &task, auto &&f) {
    f(task.source_file);
    f(task.output_dir);
    f(task.output_base_name);
    f(task.path_in_archive);
    f(task.page_number);
    f(task.processing_settings);
}

std::vector<std::string> encode_task(const PageTask &task);
PageTask decode_task(std::span<const char *const> encoded_fields);
