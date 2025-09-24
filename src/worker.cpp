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
    if (argc < 6) {
        std::cerr << "Worker error: Insufficient arguments." << std::endl;
        return 1;
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

    // A simple logger that prints to standard output.
    auto logger = [](const std::string &msg) { std::cout << msg << std::endl; };

    try {
        vips::VImage image;
        if (task.page_number != -1) {
            image = load_pdf_page(task, logger);
        }
        else if (!task.path_in_archive.empty()) {
            image = load_archive_image(task, logger);
        }
        else {
            logger(
                "Worker error: Invalid task arguments for "
                + task.source_file.string()
            );
            return 1;
        }
        process_vimage(image, task.output_dir, task.output_base_name, logger);
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
