#include "include/options.hpp"
#include "include/ui_constants.hpp"
#include "include/window_util.hpp"
#include "qcheckbox.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QWidget>
#include <thread>

#if defined(PDFIUM_ENABLED)
void add_pdf_pixel_density_widget(QStyle *style, Options *options) {
    options->pdf_pixel_density_spin_box = new DensitySpinBox();
    options->pdf_pixel_density_spin_box->setVisible(false);

    auto label = new QLabel("PDF pixel density");

    options->pdf_pixel_density_combo_box = create_combo_box(
        {"Standard (300\u202fPPI, fast)",
         "High (600\u202fPPI)",
         "Ultra (1200\u202fPPI, recommended)",
         "Custom"},
        "Standard (300\u202fPPI, fast)"
    );

    auto control_container = create_control_with_info(
        style, options->pdf_pixel_density_combo_box, PDF_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);

    options->pdf_options_container = new QWidget();
    auto pdf_layout = new QFormLayout(options->pdf_options_container);
    pdf_layout->setContentsMargins(25, 0, 0, 0);
    pdf_layout->setHorizontalSpacing(10);
    pdf_layout->setLabelAlignment(Qt::AlignLeft);

    pdf_layout->addRow(options->pdf_pixel_density_spin_box);

    options->settings_layout->addWidget(options->pdf_options_container);
}
#endif

void add_convert_to_greyscale_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Convert pages to greyscale");
    options->convert_to_greyscale = new QCheckBox("Enable");
    options->convert_to_greyscale->setChecked(true);
    auto control_container = create_control_with_info(
        style, options->convert_to_greyscale, GREYSCALE_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_double_page_spread_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Two-page spreads");
    options->double_page_spread_combo_box = create_combo_box(
        {"Rotate page",
         "Split into two pages",
         "Rotate and split",
         "Do nothing"},
        "Do nothing"
    );
    auto control_container = create_control_with_info(
        style, options->double_page_spread_combo_box, DOUBLE_PAGE_SPREAD_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);

    // Rotation options
    options->rotation_options_container = new QWidget();
    auto rotation_layout = new QFormLayout(options->rotation_options_container);
    rotation_layout->setContentsMargins(25, 0, 0, 0);
    rotation_layout->setHorizontalSpacing(10);
    rotation_layout->setLabelAlignment(Qt::AlignLeft);

    auto rotation_label = new QLabel("Rotation direction");
    options->rotation_direction_combo_box = new QComboBox();
    options->rotation_direction_combo_box->addItems(
        {"Clockwise", "Counterclockwise"}
    );

    rotation_layout->addRow(
        rotation_label, options->rotation_direction_combo_box
    );

    options->settings_layout->addWidget(options->rotation_options_container);
}

void add_linear_light_resampling_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Linear-light resampling");
    options->linear_light_resampling_label = label;

    auto check_box = new QCheckBox("Enable");
    options->linear_light_resampling_check_box = check_box;

    auto control_container = create_control_with_info(
        style, check_box, LINEAR_LIGHT_RESAMPLING_TOOLTIP
    );
    options->linear_light_resampling_container = control_container;

    options->settings_layout->addRow(label, control_container);
}

void add_remove_spine_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Remove spines");
    options->remove_spine_check_box = new QCheckBox("Enable");
    auto control_container = create_control_with_info(
        style, options->remove_spine_check_box, REMOVE_SPINE_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_contrast_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Stretch contrast");
    options->contrast_check_box = new QCheckBox("Enable");
    options->contrast_check_box->setChecked(true);
    auto control_container = create_control_with_info(
        style, options->contrast_check_box, CONTRAST_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_scaling_widgets(QStyle *style, Options *options) {
    auto label = new QLabel("Scale pages");
    options->scale_pages_label = label;
    options->enable_image_scaling_check_box = new QCheckBox("Enable");
    auto enable_container = create_control_with_info(
        style, options->enable_image_scaling_check_box, SCALE_TOOLTIP
    );
    options->scale_pages_container = enable_container;

    options->settings_layout->addRow(label, enable_container);

    options->scaling_options_container = new QWidget();
    auto scaling_layout = new QFormLayout(options->scaling_options_container);
    scaling_layout->setContentsMargins(25, 0, 0, 0);
    scaling_layout->setHorizontalSpacing(10);
    scaling_layout->setLabelAlignment(Qt::AlignLeft);

    // Width
    options->width_spin_box = new QSpinBox();
    options->width_spin_box->setRange(100, 4'000);
    options->width_spin_box->setSingleStep(100);
    options->width_spin_box->setValue(1440);
    auto width_label = new QLabel("Max width");
    scaling_layout->addRow(width_label, options->width_spin_box);

    // Height
    options->height_spin_box = new QSpinBox();
    options->height_spin_box->setRange(100, 4'000);
    options->height_spin_box->setSingleStep(100);
    options->height_spin_box->setValue(1920);
    auto height_label = new QLabel("Max height");
    scaling_layout->addRow(height_label, options->height_spin_box);

    // Resampler
    auto resampler_label = new QLabel("Resampler");
    options->resampler_combo_box = new QComboBox();
    options->resampler_combo_box->addItems(
        {"Bicubic interpolation",
         "Bilinear interpolation",
         "Lanczos 2",
         "Lanczos 3",
         "Magic Kernel Sharp 2013",
         "Magic Kernel Sharp 2021",
         "Mitchell",
         "Nearest neighbour"}
    );
    options->resampler_combo_box->setCurrentText("Magic Kernel Sharp 2021");
    auto resampler_container = create_control_with_info(
        style, options->resampler_combo_box, RESAMPLER_TOOLTIP
    );
    scaling_layout->addRow(resampler_label, resampler_container);

    options->settings_layout->addWidget(options->scaling_options_container);
}

void add_quantization_widgets(QStyle *style, Options *options) {
    auto label = new QLabel("Quantize pages");
    options->quantize_pages_label = label;

    options->enable_image_quantization_check_box = new QCheckBox("Enable");
    options->enable_image_quantization_check_box->setChecked(true);
    auto enable_container = create_control_with_info(
        style, options->enable_image_quantization_check_box, QUANTIZE_TOOLTIP
    );
    options->quantize_pages_container = enable_container;

    options->settings_layout->addRow(label, enable_container);

    options->quantization_options_container = new QWidget();
    auto quantization_layout
        = new QFormLayout(options->quantization_options_container);
    quantization_layout->setContentsMargins(25, 0, 0, 0);
    quantization_layout->setHorizontalSpacing(10);
    quantization_layout->setLabelAlignment(Qt::AlignLeft);

    // Bit depth
    auto bit_depth_label = new QLabel("Bit depth");
    options->bit_depth_combo_box = new QComboBox();
    options->bit_depth_combo_box->addItems(
        {"1 (2 colours)",
         "2 (4 colours)",
         "4 (16 colours)",
         "8 (256 colours)",
         "16 (65\u202f536 colours)"}
    );
    options->bit_depth_combo_box->setCurrentIndex(2);
    auto bit_depth_container = create_control_with_info(
        style, options->bit_depth_combo_box, BIT_DEPTH_TOOLTIP
    );
    quantization_layout->addRow(bit_depth_label, bit_depth_container);

    // Dithering
    auto dithering_label = new QLabel("Dithering");
    options->dithering_spin_box = new QDoubleSpinBox();
    options->dithering_spin_box->setRange(0.0, 1.0);
    options->dithering_spin_box->setSingleStep(0.1);
    options->dithering_spin_box->setValue(1.0);
    auto dithering_container = create_control_with_info(
        style, options->dithering_spin_box, DITHERING_TOOLTIP
    );
    quantization_layout->addRow(dithering_label, dithering_container);

    options->settings_layout->addWidget(
        options->quantization_options_container
    );
}

void add_image_format_widgets(QStyle *style, Options *options) {
    options->image_format_combo_box
        = create_combo_box({"AVIF", "JPEG", "JPEG XL", "PNG", "WebP"}, "PNG");
    auto image_format_label = new QLabel("Image format");
    options->image_format_label = image_format_label;
    auto image_format_container = create_control_with_info(
        style, options->image_format_combo_box, IMG_FORMAT_TOOLTIP
    );
    options->image_format_container = image_format_container;

    options->settings_layout->addRow(
        image_format_label, image_format_container
    );

    auto image_format_options_container = new QWidget();
    options->image_format_options_container = image_format_options_container;
    auto image_format_layout = new QFormLayout(image_format_options_container);
    image_format_layout->setContentsMargins(25, 0, 0, 0);
    image_format_layout->setHorizontalSpacing(10);
    image_format_layout->setLabelAlignment(Qt::AlignLeft);

    // Compression type
    auto compression_type_label = new QLabel("Compression type");
    options->image_compression_type_label = compression_type_label;
    options->image_compression_type_combo_box = new QComboBox();
    options->image_compression_type_combo_box->addItems({"Lossless", "Lossy"});
    options->image_compression_type_combo_box->setCurrentText("Lossless");
    options->image_compression_type_label->setVisible(false);
    options->image_compression_type_combo_box->setVisible(false);
    auto image_compression_type_pair = create_control_with_info_pair(
        style,
        options->image_compression_type_combo_box,
        IMAGE_COMPRESSION_TYPE_TOOLTIP
    );
    image_format_layout->addRow(
        options->image_compression_type_label, image_compression_type_pair.first
    );
    options->image_compression_type_tooltip
        = image_compression_type_pair.second;
    options->image_compression_type_tooltip->setVisible(false);

    // Compression effort
    options->image_compression_label = new QLabel("Compression effort");
    options->image_compression_spin_box = new QSpinBox();
    options->image_compression_spin_box->setRange(0, 9);
    options->image_compression_spin_box->setSingleStep(1);
    options->image_compression_spin_box->setValue(6);
    image_format_layout->addRow(
        options->image_compression_label, options->image_compression_spin_box
    );

    // Quality label container (for dynamic switching)
    auto quality_label_container = new QWidget();
    auto quality_label_hbox = new QHBoxLayout(quality_label_container);
    quality_label_hbox->setContentsMargins(0, 0, 0, 0);
    quality_label_hbox->setSpacing(0);

    options->image_quality_label_original = new QLabel("Quality");
    options->image_quality_label_jpeg_xl = new QComboBox();
    options->image_quality_label_jpeg_xl->addItems({"Distance", "Quality"});
    options->image_quality_label_jpeg_xl->setCurrentText("Distance");
    options->image_quality_label_jpeg_xl->setVisible(false);

    quality_label_hbox->addWidget(options->image_quality_label_original);
    quality_label_hbox->addWidget(options->image_quality_label_jpeg_xl);

    // Quality spin box
    options->image_quality_spin_box = new QDoubleSpinBox();
    options->image_quality_spin_box->setRange(0, 100);
    options->image_quality_spin_box->setSingleStep(1);
    options->image_quality_spin_box->setValue(50);
    options->image_quality_spin_box->setDecimals(0);
    options->image_quality_spin_box->setVisible(false);
    options->image_quality_label = options->image_quality_label_original;
    options->image_quality_label->setVisible(false); // Initial hidden state

    auto image_quality_jpeg_xl_pair = create_control_with_info_pair(
        style, options->image_quality_spin_box, IMAGE_QUALITY_JPEG_XL_TOOLTIP
    );
    image_format_layout->addRow(
        quality_label_container, image_quality_jpeg_xl_pair.first
    );
    options->image_quality_jpeg_xl_tooltip = image_quality_jpeg_xl_pair.second;
    options->image_quality_jpeg_xl_tooltip->setVisible(false);

    options->settings_layout->addWidget(image_format_options_container);
}

void add_parallel_workers_widget(QStyle *, Options *options) {
    auto label = new QLabel("Parallel jobs");
    options->workers_label = label;

    options->workers_spin_box = new QSpinBox();
    auto threads = std::thread::hardware_concurrency();
    options->workers_spin_box->setRange(1, threads);
    options->workers_spin_box->setValue(threads);

    options->settings_layout->addRow(label, options->workers_spin_box);
}
