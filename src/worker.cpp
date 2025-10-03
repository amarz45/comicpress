#include "include/processing.hpp"
#include "include/task.hpp"

#include <fpdfview.h>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vips/vips8>

// Helper function to parse arguments with error handling and cleanup
template <typename T>
T parse_arg(const std::string &arg_str, const std::string &error_msg) {
    try {
        if constexpr (std::is_same_v<T, int>) {
            return std::stoi(arg_str);
        }
        else if constexpr (std::is_same_v<T, float>) {
            return std::stof(arg_str);
        }
        else {
            static_assert(
                std::is_same_v<T, int> || std::is_same_v<T, float>,
                "Unsupported type for parse_arg"
            );
            throw std::invalid_argument("Unsupported type");
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << error_msg << " - " << e.what() << "\n";
        FPDF_DestroyLibrary();
        vips_shutdown();
        exit(1);
    }
}

// This is the main entry point for the worker executable. It takes task details
// as command-line arguments, performs the processing, and prints logs to
// standard output for the main application to capture.
int main(int argc, char *argv[]) {
    // Expect program name + 21 pairs of (flag, value) = 41 arguments
    if (argc != 41) {
        std::cerr << "Worker error: Expected exactly 40 arguments (21 "
                     "flag-value pairs) after program name."
                  << std::endl;
        return 1;
    }

    // Parse arguments into a map
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; i += 2) {
        std::string flag = argv[i];
        std::string value = argv[i + 1];
        args[flag] = value;
    }

    // Initialize libraries required for processing.
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
        return 1;
    }
    vips_concurrency_set(1);
    FPDF_InitLibrary();

    // Reconstruct the PageTask from command-line arguments.
    PageTask task;
    try {
        task.source_file = args.at("-source_file");
        task.output_dir = args.at("-output_dir");
        task.output_base_name = args.at("-output_base_name");

        task.page_number = parse_arg<int>(
            args.at("-page_number"), "Invalid page number format"
        );

        task.path_in_archive
            = args.at("-path_in_archive"); // Can be empty string

        task.pdf_pixel_density = parse_arg<int>(
            args.at("-pdf_pixel_density"), "Invalid PDF pixel density"
        );

        task.double_page_spread_action
            = (DoublePageSpreadActions)parse_arg<int>(
                args.at("-double_page_spread_actions"),
                "Invalid double page spread options"
            );

        task.remove_spine
            = parse_arg<int>(args.at("-remove_spine"), "Invalid remove spine")
           != 0;

        task.stretch_page_contrast = parse_arg<int>(
                                         args.at("-stretch_page_contrast"),
                                         "Invalid stretch page contrast"
                                     )
                                  != 0;
        task.scale_pages
            = parse_arg<int>(args.at("-scale_pages"), "Invalid scale pages")
           != 0;

        task.page_width
            = parse_arg<int>(args.at("-page_width"), "Invalid page width");
        task.page_height
            = parse_arg<int>(args.at("-page_height"), "Invalid page height");
        task.page_resampler = static_cast<VipsKernel>(
            parse_arg<int>(args.at("-page_resampler"), "Invalid page resampler")
        );

        task.quantize_pages
            = parse_arg<int>(
                  args.at("-quantize_pages"), "Invalid quantize pages"
              )
           != 0;

        task.bit_depth
            = parse_arg<int>(args.at("-bit_depth"), "Invalid bit depth");
        task.dither
            = parse_arg<float>(args.at("-dither"), "Invalid dither value");

        task.image_format = args.at("-image_format");
        task.is_lossy
            = parse_arg<int>(args.at("-is_lossy"), "Invalid is lossy") != 0;
        task.quality_type_is_distance
            = parse_arg<int>(
                  args.at("-quality_type_is_distance"),
                  "Invalid quality type is distance"
              )
           != 0;

        task.compression_effort = parse_arg<int>(
            args.at("-compression_effort"), "Invalid compression effort"
        );
    }
    catch (const std::out_of_range &e) {
        std::cerr << "Worker error: Missing required argument - " << e.what()
                  << "\n";
        FPDF_DestroyLibrary();
        vips_shutdown();
        return 1;
    }

    // A simple logger that prints to standard output.
    auto logger = [](const std::string &msg) { std::cout << msg << std::endl; };

    try {
        vips::VImage image;
        if (!task.path_in_archive.empty()) {
            image = load_archive_image(task);
        }
        else {
            image = load_pdf_page(task);
        }
        process_vimage(image, task, logger);
    }
    catch (const std::exception &e) {
        logger(
            "Worker error processing task for "
            + task.source_file.stem().string() + ": " + e.what()
        );
        FPDF_DestroyLibrary();
        vips_shutdown();
        return 1;
    }

    // Clean up libraries.
    FPDF_DestroyLibrary();
    vips_shutdown();

    return 0;
}
