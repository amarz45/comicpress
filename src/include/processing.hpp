#pragma once

#include "task.hpp"
#include <functional>
#include <string>
#include <vips/vips8>

using Logger = const std::function<void(const std::string &)> &;

vips::VImage load_pdf_page(const PageTask &task, Logger log);
vips::VImage load_archive_image(const PageTask &task, Logger log);

void process_vimage(vips::VImage img, PageTask task, Logger log);
