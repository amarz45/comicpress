#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

struct NoImagesError : std::runtime_error {
    NoImagesError() : std::runtime_error("no images") {
    }
};

struct NoImagesError;

void create_epub(
    const fs::path &image_dir,
    const fs::path &output_path,
    const std::string &title
);

void create_cbz(const fs::path &image_dir, const fs::path &output_path);
