#pragma once

#include "task.hpp"
#include <fpdfview.h>
#include <functional>
#include <string>
#include <vips/vips8>

using Logger = const std::function<void(const std::string &)> &;

struct LoadPageReturn {
    vips::VImage image;
    bool is_originally_greyscale;
};

LoadPageReturn load_pdf_page(const PageTask &task);
LoadPageReturn load_archive_image(const PageTask &task);
vips::VImage get_vips_img_from_pdf_page(
    FPDF_DOCUMENT doc,
    FPDF_PAGE page,
    int page_number,
    int colour_mode,
    int bands,
    double ppi,
    unsigned int render_flags
);
vips::VImage remove_uniform_middle_columns(const vips::VImage &img);
bool is_preview_greyscale(FPDF_DOCUMENT doc, FPDF_PAGE page, int page_number);
bool is_greyscale(vips::VImage img, int threshold);
bool should_image_rotate(
    double image_width,
    double image_height,
    double display_width,
    double display_height
);
bool is_uniform_column(const vips::VImage &img, int col, double threshold);

void process_vimage(LoadPageReturn page_info, PageTask task, Logger log);
