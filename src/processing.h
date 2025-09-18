#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vips/vips8>
#include "task.h"

namespace fs = std::filesystem;

using Logger = const std::function<void(const std::string&)>&;

vips::VImage load_pdf_page(const PageTask& task, Logger log);
vips::VImage load_archive_image(const PageTask& task, Logger log);

void process_image_file(
    const fs::path &input_path,
    const fs::path &output_dir,
    const std::string& base_name,
    Logger log
);

void process_vimage(
    vips::VImage img,
    const fs::path &output_dir,
    const std::string& base_name,
    Logger log
);
