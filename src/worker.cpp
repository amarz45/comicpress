// worker.cpp
#include "worker.h"
#include "processing.h"
#include <functional>
#include <vips/vips8>

Worker::Worker(const PageTask& task) : task(task) {}

void Worker::run() {
    auto logger = [this](const std::string& msg) {
        emit logMessage(QString::fromStdString(msg));
    };

    try {
        vips::VImage image;
        // Check if it's a PDF page or an archive image.
        if (task.page_number != -1) {
            image = load_pdf_page(task, logger);
        } else if (!task.path_in_archive.empty()) {
            image = load_archive_image(task, logger);
        } else {
            logger("Error: Invalid task for file " + task.source_file.stem().string());
            emit finished();
            return;
        }

        // Now process the loaded VImage.
        process_vimage(image, task.output_dir, task.output_base_name, logger);

    } catch (const std::exception& e) {
        logger("Error processing task for " + task.source_file.stem().string() + ": " + e.what());
    }

    // Signal that this one task is finished.
    emit finished();
}
