#pragma once

#include "task.hpp"
#include <fpdfview.h>
#include <functional>
#include <string>
#include <vips/vips8>

using Logger = const std::function<void(const std::string &)> &;

vips::VImage load_pdf_page(const PageTask &task, Logger log);
vips::VImage load_archive_image(const PageTask &task, Logger log);
vips::VImage get_vips_img_from_pdf_page(
    FPDF_DOCUMENT doc,
    FPDF_PAGE page,
    int page_number,
    int colour_mode,
    int bands,
    double ppi,
    unsigned int render_flags
);
bool is_preview_greyscale(FPDF_DOCUMENT doc, FPDF_PAGE page, int page_number);
bool is_greyscale(vips::VImage img, int threshold);

void process_vimage(vips::VImage img, PageTask task, Logger log);
