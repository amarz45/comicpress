#pragma once

#include "../../include/task.hpp"
#include <functional>
#include <string>
#include <vips/vips8>

using Logger = const std::function<void(const std::string &)> &;

struct LoadPageReturn {
    vips::VImage image;
    bool stretch_page_contrast;
};

LoadPageReturn load_pdf_page(const PageTask &task);
LoadPageReturn load_archive_image(const PageTask &task);

void process_vimage(LoadPageReturn page_info, PageTask task, Logger log);
