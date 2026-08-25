#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

struct ArchiveError : std::runtime_error {
    ArchiveError() : std::runtime_error("archive error") {
    }
};

struct NoImagesError : std::runtime_error {
    NoImagesError() : std::runtime_error("no images") {
    }
};

void create_epub(
    const fs::path &image_dir,
    const fs::path &output_path,
    const std::string &title
);

void create_cbz(const fs::path &image_dir, const fs::path &output_path);
