#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct PageTask {
    fs::path source_file;
    fs::path output_dir;
    std::string output_base_name;

    int page_number = -1;
    std::string path_in_archive;
};
