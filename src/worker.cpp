#include "include/processing.hpp"
#include "include/task.hpp"

#include <fpdfview.h>
#include <iostream>
#include <string>
#include <vips/vips8>

// This is the main entry point for the worker executable. It takes task details
// as command-line arguments, performs the processing, and prints logs to
// standard output for the main application to capture.
int main(int argc, char *argv[]) {
    // if (argc < 7) {
    //     std::cerr << "Worker error: Insufficient arguments." << std::endl;
    //     return 1;
    // }

    // Initialize libraries required for processing.
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
        return 1;
    }
    vips_concurrency_set(1);
    FPDF_InitLibrary();

    // Reconstruct the PageTask from command-line arguments.
    PageTask task;
    task.source_file = argv[1];
    task.output_dir = argv[2];
    task.output_base_name = argv[3];
    try {
        task.page_number = std::stoi(argv[4]);
    }
    catch (const std::invalid_argument &e) {
        std::cerr << "Worker error: Invalid page number format." << std::endl;
        return 1;
    }

    std::string path_in_archive_str = argv[5];
    if (path_in_archive_str != "NULL") {
        task.path_in_archive = path_in_archive_str;
    }

    try {
        task.pdf_pixel_density = std::stoi(argv[6]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    task.stretch_page_contrast = argv[7][0] == '1';
    task.scale_pages = argv[8][0] == '1';

    try {
        task.page_width = std::stoi(argv[9]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    try {
        task.page_height = std::stoi(argv[10]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    auto resampler = std::string(argv[11]);
    if (resampler == "Bicubic interpolation") {
        task.page_resampler = VIPS_KERNEL_CUBIC;
    }
    else if (resampler == "Bilinear interpolation") {
        task.page_resampler = VIPS_KERNEL_LINEAR;
    }
    else if (resampler == "Lanczos 2") {
        task.page_resampler = VIPS_KERNEL_LANCZOS2;
    }
    else if (resampler == "Lanczos 3") {
        task.page_resampler = VIPS_KERNEL_LANCZOS3;
    }
    else if (resampler == "Magic Kernel Sharp 2013") {
        task.page_resampler = VIPS_KERNEL_MKS2013;
    }
    else if (resampler == "Magic Kernel Sharp 2021") {
        task.page_resampler = VIPS_KERNEL_MKS2021;
    }
    else if (resampler == "Mitchell") {
        task.page_resampler = VIPS_KERNEL_MITCHELL;
    }
    else {
        task.page_resampler = VIPS_KERNEL_NEAREST;
    }

    task.quantize_pages = argv[12][0] == '1';

    try {
        task.bit_depth = std::stoi(argv[13]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    try {
        task.dither = std::stof(argv[14]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    task.image_format = argv[15];
    task.is_lossy = argv[16][0] == '1';
    task.quality_type_is_distance = argv[17][0] == '1';

    try {
        task.compression_effort = std::stoi(argv[18]);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: " << e.what() << "\n";
        return 1;
    }

    // A simple logger that prints to standard output.
    auto logger = [](const std::string &msg) { std::cout << msg << std::endl; };

    try {
        vips::VImage image;
        if (!task.path_in_archive.empty()) {
            image = load_archive_image(task, logger);
        }
        else {
            image = load_pdf_page(task, logger);
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
