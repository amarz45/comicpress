#include <gtest/gtest.h>

#include <iomanip>
#include <ostream>

#include "task.hpp"

static void PrintTo(const ProcessingSettings &settings, std::ostream *os) {
    *os << std::boolalpha
        << "\n    image_format = " << static_cast<int>(settings.image_format)
        << "\n    dither = " << settings.dither
        << "\n    quality = " << settings.quality
#if defined(PDF_ENABLED)
        << "\n    pdf_pixel_density = " << settings.pdf_pixel_density
#endif
        << "\n    page_width = " << settings.page_width
        << "\n    page_height = " << settings.page_height
        << "\n    bit_depth = " << static_cast<int>(settings.bit_depth)
        << "\n    compression_effort = " << settings.compression_effort
        << "\n    double_page_spread_action = "
        << static_cast<int>(settings.double_page_spread_action)
        << "\n    reading_direction = "
        << static_cast<int>(settings.reading_direction)
        << "\n    rotation_direction = "
        << static_cast<int>(settings.rotation_direction)
        << "\n    page_resampler = " << settings.page_resampler
        << "\n    convert_pages_to_greyscale = "
        << settings.convert_pages_to_greyscale
        << "\n    remove_spine = " << settings.remove_spine
        << "\n    stretch_page_contrast = " << settings.stretch_page_contrast
        << "\n    linear_light_resampling = "
        << settings.linear_light_resampling
        << "\n    scale_pages = " << settings.scale_pages
        << "\n    quantize_pages = " << settings.quantize_pages
        << "\n    compression_type = "
        << static_cast<int>(settings.compression_type)
        << "\n    quality_type = " << static_cast<int>(settings.quality_type);
}

static void PrintTo(const PageTask &task, std::ostream *os) {
    *os << "\n  source_file = " << task.source_file
        << "\n  output_dir = " << task.output_dir
        << "\n  output_base_name = " << std::quoted(task.output_base_name)
        << "\n  path_in_archive = " << std::quoted(task.path_in_archive)
        << "\n  page_number = " << task.page_number
        << "\n  processing_settings:";
    PrintTo(task.processing_settings, os);
}

static std::vector<const char *> as_argv(const std::vector<std::string> &args) {
    auto argv = std::vector<const char *>{};
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    return argv;
}

TEST(Task, EncodeDecodeRoundTrip1) {
    auto task = PageTask{};
    task.source_file = "/tmp/some_dir/comic.pdf";
    task.output_dir = "/tmp/out";
    task.output_base_name = "chapter 1/page_43";
    task.path_in_archive = "chapter 1/page_43.png";
    task.page_number = 42;

    task.processing_settings.image_format = ImageFormat::JPEG_XL;
    task.processing_settings.dither = 1.0;
    task.processing_settings.quality = 1.0;
#if defined(PDF_ENABLED)
    task.processing_settings.pdf_pixel_density = 1200;
#endif
    task.processing_settings.page_width = 1440;
    task.processing_settings.page_height = 1920;
    task.processing_settings.bit_depth = BitDepth::FOUR;
    task.processing_settings.compression_effort = 6;
    task.processing_settings.double_page_spread_action = DoublePageSpreadActions::ROTATE;
    task.processing_settings.reading_direction = ReadingDirection::RTL;
    task.processing_settings.rotation_direction = RotationDirection::CLOCKWISE;
    task.processing_settings.page_resampler = VIPS_KERNEL_MKS2021;
    task.processing_settings.convert_pages_to_greyscale = true;
    task.processing_settings.remove_spine = false;
    task.processing_settings.stretch_page_contrast = true;
    task.processing_settings.linear_light_resampling = false;
    task.processing_settings.scale_pages = true;
    task.processing_settings.quantize_pages = true;
    task.processing_settings.compression_type = CompressionType::LOSSLESS;
    task.processing_settings.quality_type = QualityType::DISTANCE;

    const auto encoded = encode_task(task);
    const auto decoded = decode_task(as_argv(encoded));

    EXPECT_EQ(decoded, task);
}

TEST(Task, EncodeDecodeRoundTrip2) {
    auto task = PageTask{};
    task.source_file = "/tmp/some_other_dir/comic.cbz";
    task.output_dir = "/tmp/out_other";
    task.output_base_name = "chapter 2/page_1";
    task.path_in_archive = "chapter 2/page_1.png";
    task.page_number = 0;

    task.processing_settings.image_format = ImageFormat::WEBP;
    task.processing_settings.dither = 0.0;
    task.processing_settings.quality = 0;
#if defined(PDF_ENABLED)
    task.processing_settings.pdf_pixel_density = 300;
#endif
    task.processing_settings.page_width = 1200;
    task.processing_settings.page_height = 1000;
    task.processing_settings.bit_depth = BitDepth::EIGHT;
    task.processing_settings.compression_effort = 3;
    task.processing_settings.double_page_spread_action = DoublePageSpreadActions::BOTH;
    task.processing_settings.reading_direction = ReadingDirection::LTR;
    task.processing_settings.rotation_direction = RotationDirection::COUNTERCLOCKWISE;
    task.processing_settings.page_resampler = VIPS_KERNEL_LANCZOS3;
    task.processing_settings.convert_pages_to_greyscale = false;
    task.processing_settings.remove_spine = true;
    task.processing_settings.stretch_page_contrast = false;
    task.processing_settings.linear_light_resampling = true;
    task.processing_settings.scale_pages = false;
    task.processing_settings.quantize_pages = false;
    task.processing_settings.compression_type = CompressionType::LOSSY;
    task.processing_settings.quality_type = QualityType::QUALITY;

    const auto encoded = encode_task(task);
    const auto decoded = decode_task(as_argv(encoded));

    EXPECT_EQ(decoded, task);
}
