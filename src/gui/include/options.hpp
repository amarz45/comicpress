#pragma once

#include "window.hpp"
#include <QStyle>

void add_pdf_pixel_density_widget(QStyle *style, Options *options);
void add_convert_to_greyscale_widget(QStyle *style, Options *options);
void add_double_page_spread_widget(QStyle *style, Options *options);
void add_remove_spine_widget(QStyle *style, Options *options);
void add_contrast_widget(QStyle *style, Options *options);
void add_scaling_widgets(QStyle *style, Options *options);
void add_quantization_widgets(QStyle *style, Options *options);
void add_image_format_widgets(QStyle *style, Options *options);
void add_parallel_workers_widget(QStyle *style, Options *options);
