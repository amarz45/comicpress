#include "include/options.hpp"
#include "../include/task.hpp"
#include "include/ui_constants.hpp"
#include "include/window_util.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVariant>
#include <QWidget>
#include <Qt>

#include <vips/vips8>

#include <thread>
#include <utility>

static QFormLayout *create_form_layout(QWidget *container);

static constexpr auto OUTPUT_FORMATS
    = std::to_array<ComboBoxVariant<OutputFormat>>({
        {"epub", "EPUB", OutputFormat::EPUB},
        {"cbz", "CBZ", OutputFormat::CBZ},
    });

static constexpr auto READING_DIRECTIONS
    = std::to_array<ComboBoxVariant<ReadingDirection>>({
        {"ltr", "Left-to-right", ReadingDirection::LTR},
        {"rtl", "Right-to-left", ReadingDirection::RTL},
    });

static constexpr auto PDF_QUALITY_OPTIONS
    = std::to_array<ComboBoxVariant<PdfQuality>>({
        {"standard", "Standard (300\u202fPPI, fast)", PdfQuality::STANDARD},
        {"high", "High (600\u202fPPI)", PdfQuality::HIGH},
        {"ultra", "Ultra (1200\u202fPPI, recommended)", PdfQuality::ULTRA},
        {"custom", "Custom", PdfQuality::CUSTOM},
    });

static constexpr auto DOUBLE_PAGE_SPREAD_ACTIONS
    = std::to_array<ComboBoxVariant<DoublePageSpreadActions>>({
        {"none", "Do nothing", DoublePageSpreadActions::NONE},
        {"rotate", "Rotate page", DoublePageSpreadActions::ROTATE},
        {"split", "Split into two pages", DoublePageSpreadActions::SPLIT},
        {"rotate_and_split", "Rotate and split", DoublePageSpreadActions::BOTH},
    });

static constexpr auto ROTATION_DIRECTIONS
    = std::to_array<ComboBoxVariant<RotationDirection>>({
        {"clockwise", "Clockwise", RotationDirection::CLOCKWISE},
        {"counterclockwise",
         "Counterclockwise",
         RotationDirection::COUNTERCLOCKWISE},
    });

static constexpr auto RESAMPLERS = std::to_array<ComboBoxVariant<VipsKernel>>({
    {"bicubic", "Bicubic interpolation", VIPS_KERNEL_CUBIC},
    {"bilinear", "Bilinear interpolation", VIPS_KERNEL_LINEAR},
    {"lanczos_2", "Lanczos 2", VIPS_KERNEL_LANCZOS2},
    {"lanczos_3", "Lanczos 3", VIPS_KERNEL_LANCZOS3},
    {"mks2013", "Magic Kernel Sharp 2013", VIPS_KERNEL_MKS2013},
    {"mks2021", "Magic Kernel Sharp 2021", VIPS_KERNEL_MKS2021},
    {"mitchell", "Mitchell", VIPS_KERNEL_MITCHELL},
    {"nearest_neighbour", "Nearest neighbour", VIPS_KERNEL_NEAREST},
});

static constexpr auto BIT_DEPTH_OPTIONS
    = std::to_array<ComboBoxVariant<BitDepth>>({
        {"one", "1 (2 colours)", BitDepth::ONE},
        {"two", "2 (4 colours)", BitDepth::TWO},
        {"four", "4 (16 colours)", BitDepth::FOUR},
        {"eight", "8 (256 colours)", BitDepth::EIGHT},
        {"sixteen", "16 (65\u202f536 colours)", BitDepth::SIXTEEN},
    });

static constexpr auto IMAGE_FORMATS
    = std::to_array<ComboBoxVariant<ImageFormat>>({
        {"avif", "AVIF", ImageFormat::AVIF},
        {"jpeg", "JPEG", ImageFormat::JPEG},
        {"jpeg_xl", "JPEG XL", ImageFormat::JPEG_XL},
        {"png", "PNG", ImageFormat::PNG},
        {"webp", "WebP", ImageFormat::WEBP},
    });

static constexpr auto COMPRESSION_TYPES
    = std::to_array<ComboBoxVariant<CompressionType>>({
        {"lossless", "Lossless", CompressionType::LOSSLESS},
        {"lossy", "Lossy", CompressionType::LOSSY},
    });

static constexpr auto QUALITY_TYPES
    = std::to_array<ComboBoxVariant<QualityType>>({
        {"distance", "Distance", QualityType::DISTANCE},
        {"quality", "Quality", QualityType::QUALITY},
    });

void add_output_format_widget(QStyle *style, Options *options) {
    auto output_format_label = new QLabel("Output format");
    options->output_format_combo_box
        = create_combo_box(OUTPUT_FORMATS, OutputFormat::EPUB);
    auto output_format_container = create_control_with_info(
        style, options->output_format_combo_box, OUTPUT_FORMAT_TOOLTIP
    );

    options->settings_layout->addRow(
        output_format_label, output_format_container
    );

    options->output_format_container = new QWidget();
    auto output_format_layout
        = create_form_layout(options->output_format_container);
    auto reading_direction_label = new QLabel("Reading direction");
    options->reading_direction_combo_box
        = create_combo_box(READING_DIRECTIONS, ReadingDirection::LTR);
    output_format_layout->addRow(
        reading_direction_label, options->reading_direction_combo_box
    );

    options->settings_layout->addWidget(options->output_format_container);
}

#if defined(PDF_ENABLED)
void add_pdf_pixel_density_widget(QStyle *style, Options *options) {
    options->pdf_pixel_density_spin_box = create_spin_box<DensitySpinBox>();
    options->pdf_pixel_density_spin_box->setVisible(false);

    auto label = new QLabel("PDF pixel density");
    options->pdf_pixel_density_label = label;

    options->pdf_pixel_density_combo_box
        = create_combo_box(PDF_QUALITY_OPTIONS, PdfQuality::STANDARD);

    auto control_pair = create_control_with_info_pair(
        style, options->pdf_pixel_density_combo_box, PDF_TOOLTIP
    );
    options->settings_layout->addRow(label, control_pair.first);
    options->pdf_pixel_density_tooltip = control_pair.second;

    options->pdf_options_container = new QWidget();
    auto pdf_layout = create_form_layout(options->pdf_options_container);
    pdf_layout->addRow(options->pdf_pixel_density_spin_box);

    options->pdf_pixel_density_label->setVisible(false);
    options->pdf_pixel_density_combo_box->setVisible(false);
    options->pdf_pixel_density_tooltip->setVisible(false);
    options->pdf_options_container->setVisible(false);

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
        DOUBLE_PAGE_SPREAD_ACTIONS, DoublePageSpreadActions::NONE
    );
    auto control_container = create_control_with_info(
        style, options->double_page_spread_combo_box, DOUBLE_PAGE_SPREAD_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);

    // Rotation options
    options->rotation_options_container = new QWidget();
    auto rotation_layout
        = create_form_layout(options->rotation_options_container);

    auto rotation_label = new QLabel("Rotation direction");

    options->rotation_direction_combo_box
        = create_combo_box(ROTATION_DIRECTIONS, RotationDirection::CLOCKWISE);

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
    auto scaling_layout
        = create_form_layout(options->scaling_options_container);

    const auto dimension_min = 100;
    const auto dimension_max = 4000;
    const auto dimension_step = 100;

    // Width
    options->width_spin_box = create_spin_box({
        .range = {dimension_min, dimension_max},
        .step = dimension_step,
        .value = 1440,
    });
    auto width_label = new QLabel("Max width");
    scaling_layout->addRow(width_label, options->width_spin_box);

    // Height
    options->height_spin_box = create_spin_box({
        .range = {dimension_min, dimension_max},
        .step = dimension_step,
        .value = 1920,
    });
    auto height_label = new QLabel("Max height");
    scaling_layout->addRow(height_label, options->height_spin_box);

    auto resampler_label = new QLabel("Resampler");

    options->resampler_combo_box
        = create_combo_box(RESAMPLERS, VIPS_KERNEL_MKS2021);

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
        = create_form_layout(options->quantization_options_container);

    // Bit depth
    auto bit_depth_label = new QLabel("Bit depth");
    options->bit_depth_combo_box
        = create_combo_box(BIT_DEPTH_OPTIONS, BitDepth::FOUR);
    auto bit_depth_container = create_control_with_info(
        style, options->bit_depth_combo_box, BIT_DEPTH_TOOLTIP
    );
    quantization_layout->addRow(bit_depth_label, bit_depth_container);

    // Dithering
    auto dithering_label = new QLabel("Dithering");
    options->dithering_spin_box = create_spin_box<QDoubleSpinBox>(
        {.range = {0.0, 1.0}, .step = 0.1, .value = 1.0}
    );
    auto dithering_container = create_control_with_info(
        style, options->dithering_spin_box, DITHERING_TOOLTIP
    );
    quantization_layout->addRow(dithering_label, dithering_container);

    options->settings_layout->addWidget(
        options->quantization_options_container
    );
}

void add_image_format_widgets(QStyle *style, Options *options) {
    options->image_format_label = new QLabel("Image format");

    options->image_format_combo_box
        = create_combo_box(IMAGE_FORMATS, ImageFormat::PNG);

    auto image_format_container = create_control_with_info(
        style, options->image_format_combo_box, IMG_FORMAT_TOOLTIP
    );
    options->image_format_container = image_format_container;

    options->settings_layout->addRow(
        options->image_format_label, image_format_container
    );

    options->image_format_options_container = new QWidget();
    auto image_format_layout
        = create_form_layout(options->image_format_options_container);

    // Compression type
    options->image_compression_type_combo_box
        = create_combo_box(COMPRESSION_TYPES, CompressionType::LOSSLESS);

    auto compression_type_label = new QLabel("Compression type");
    options->image_compression_type_label = compression_type_label;
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
    options->image_compression_spin_box
        = create_spin_box({.range = {0, 9}, .step = 1, .value = 6});
    image_format_layout->addRow(
        options->image_compression_label, options->image_compression_spin_box
    );

    // Quality label container (for dynamic switching)
    auto quality_label_container = new QWidget();
    auto quality_label_hbox = new QHBoxLayout(quality_label_container);
    quality_label_hbox->setContentsMargins(0, 0, 0, 0);
    quality_label_hbox->setSpacing(0);

    // Quality/distance combo box
    options->image_quality_label_jpeg_xl
        = create_combo_box(QUALITY_TYPES, QualityType::DISTANCE);
    options->image_quality_label_jpeg_xl->setVisible(false);

    options->image_quality_label_original = new QLabel("Quality");

    quality_label_hbox->addWidget(options->image_quality_label_original);
    quality_label_hbox->addWidget(options->image_quality_label_jpeg_xl);

    // Quality spin box
    options->image_quality_spin_box = create_spin_box<QDoubleSpinBox>(
        {.range = {0, 100}, .step = 1, .value = 50}
    );
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

    options->settings_layout->addWidget(
        options->image_format_options_container
    );
}

void add_parallel_workers_widget(QStyle *, Options *options) {
    auto label = new QLabel("Parallel jobs");
    options->workers_label = label;

    auto threads
        = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    options->workers_spin_box
        = create_spin_box({.range = {1, threads}, .step = 1, .value = threads});

    options->settings_layout->addRow(label, options->workers_spin_box);
}

static QFormLayout *create_form_layout(QWidget *container) {
    auto layout = new QFormLayout(container);
    layout->setContentsMargins(25, 0, 0, 0);
    layout->setHorizontalSpacing(10);
    layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    return layout;
}
