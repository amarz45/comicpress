#include "include/worker.hpp"
#include "../include/task.hpp"
#include "include/processing.hpp"

#include <iostream>
#include <string>

#if defined(PDF_ENABLED)
#include <fpdfview.h>
#endif

// This is the main entry point for the worker executable. It takes task details
// as command-line arguments, performs the processing, and prints logs to
// standard output for the main application to capture.
int worker_main(int argc, char **argv) {
    // Initialize libraries required for processing.
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
        return 1;
    }
    vips_concurrency_set(1);
#if defined(PDF_ENABLED)
    FPDF_InitLibrary();
#endif

    auto task = PageTask{};
    try {
        auto worker_arguments = std::span(argv + 1, argv + argc);
        task = decode_task(worker_arguments);
    }
    catch (const std::exception &e) {
        std::cerr << "Worker error: invalid arguments: " << e.what() << "\n";
#if defined(PDF_ENABLED)
        FPDF_DestroyLibrary();
#endif
        vips_shutdown();
        return 1;
    }

    // A simple logger that prints to standard output.
    auto logger = [](const std::string &msg) { std::cout << msg << '\n'; };

    try {
        LoadPageReturn page_info;
        if (!task.path_in_archive.empty()) {
            page_info = load_archive_image(task);
        }
        else {
#if defined(PDF_ENABLED)
            page_info = load_pdf_page(task);
#else
            exit(1);
#endif
        }
        process_vimage(page_info, task, logger);
    }
    catch (const std::exception &e) {
        logger(
            "Worker error processing task for "
            + task.source_file.stem().string() + ": " + e.what()
        );
#if defined(PDF_ENABLED)
        FPDF_DestroyLibrary();
#endif
        vips_shutdown();
        return 1;
    }

// Clean up libraries.
#if defined(PDF_ENABLED)
    FPDF_DestroyLibrary();
#endif
    vips_shutdown();

    return 0;
}
