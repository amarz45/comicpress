#include "include/window.hpp"
#include "../include/task.hpp"
#include "include/display_presets.hpp"
#include "include/options.hpp"
#include "include/output_formats.hpp"
#include "include/ui_constants.hpp"
#include "include/window_util.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringList>
#include <QStyle>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>
#include <Qt>

#include <utility>
#include <vips/vips8>

#include <chrono>
#include <cmath>

namespace fs = std::filesystem;

Window::Window(QWidget *parent) : QMainWindow(parent), eta_samples_(5) {
    // Timer
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &Window::update_time_labels);
    start_time_ = std::nullopt;
    last_eta_time_ = std::nullopt;
    images_since_last_eta_ = 0;
    is_processing_cancelled_ = false;
    is_programmatically_changing_values_ = false;
    temp_base_dir_ = "";

    setWindowTitle("Comicpress");
    central_widget_ = new QWidget(this);
    setCentralWidget(central_widget_);

    setup_ui();
    set_display_preset("None", "");
    on_enable_image_scaling_changed(
        options_.enable_image_scaling_check_box->checkState()
    );
    on_double_page_spread_changed(
        options_.double_page_spread_combo_box->currentText()
    );
    on_output_format_combo_box_changed(
        options_.output_format_combo_box->currentText()
    );
    connect_signals();

    restore_output_dir();
}

void Window::setup_ui() {
    auto container_layout = new QHBoxLayout(central_widget_);
    container_layout->setContentsMargins(40, 20, 40, 20);
    auto content_widget = new QWidget();
    container_layout->setAlignment(Qt::AlignTop);
    content_widget->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Preferred
    );

    auto io_group = create_io_group();
    options_.settings_group = create_settings_group();

    progress_bars_group_ = new QGroupBox("File progress");
    progress_bars_group_->setFlat(true);
    progress_bars_layout_ = new QVBoxLayout(progress_bars_group_);
    progress_bars_group_->setVisible(false);

    create_log_group();

    auto tabs = new QTabWidget();
    tabs->setDocumentMode(true);

    auto io_scroll = new QScrollArea();
    io_scroll->setWidget(io_group);
    io_scroll->setWidgetResizable(true);
    io_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    io_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabs->addTab(io_scroll, "Files");

    auto settings_scroll = new QScrollArea();
    settings_scroll->setWidget(options_.settings_group);
    settings_scroll->setWidgetResizable(true);
    settings_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    settings_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabs->addTab(settings_scroll, "Options");

    auto action_group = new QWidget();
    auto action_layout = new QHBoxLayout(action_group);
    start_button_ = new QPushButton("Start");
    cancel_button_ = new QPushButton("Cancel");
    start_button_->setEnabled(false);
    cancel_button_->setEnabled(false);
    action_layout->addStretch();
    action_layout->addWidget(start_button_);
    action_layout->addWidget(cancel_button_);

    main_layout_ = new QVBoxLayout(content_widget);
    main_layout_->setContentsMargins(0, 0, 0, 0);

    main_layout_->addWidget(tabs);
    main_layout_->addSpacing(20);

    auto separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    main_layout_->addWidget(separator);

    main_layout_->addWidget(progress_bars_group_);
    main_layout_->addWidget(log_group_);
    main_layout_->addItem(
        new QSpacerItem(1000, 0, QSizePolicy::Preferred, QSizePolicy::Fixed)
    );
    main_layout_->addWidget(action_group);

    container_layout->addStretch(1);
    container_layout->addWidget(content_widget);
    container_layout->addStretch(1);
}

QGroupBox *Window::create_io_group() {
    auto io_group = new QGroupBox();
    io_group->setFlat(true);
    auto io_layout = new QVBoxLayout(io_group);
    io_layout->setContentsMargins(0, 10, 0, 0);
    auto file_buttons_layout = new QHBoxLayout();

    file_list_ = new QListWidget();
    file_list_->setFont(QFont("monospace"));
    file_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    file_list_->setMaximumHeight(500);

    add_files_button_ = new QPushButton("Add input files");
    remove_selected_button_ = new QPushButton("Remove selected");
    clear_all_button_ = new QPushButton("Remove all");

    remove_selected_button_->setVisible(false);
    clear_all_button_->setVisible(false);

    file_buttons_layout->addWidget(add_files_button_);
    file_buttons_layout->addWidget(remove_selected_button_);
    file_buttons_layout->addWidget(clear_all_button_);
    file_buttons_layout->addStretch();

    io_layout->addLayout(file_buttons_layout);
    io_layout->addWidget(file_list_, 1);

    auto output_layout = new QHBoxLayout();

    output_dir_field_ = new QLineEdit();
    output_dir_field_->setPlaceholderText(tr("No output folder chosen"));

    // The folder must be picked through the file dialog so the sandbox grants
    // access to it; a typed-in path would not be writable.
    output_dir_field_->setReadOnly(true);
    output_dir_field_->setCursor(Qt::PointingHandCursor);

    // Folder icon inside the field.
    auto browse_icon = style()->standardIcon(QStyle::SP_DirIcon);
    output_dir_field_->addAction(browse_icon, QLineEdit::LeadingPosition);

    browse_output_button_ = new QPushButton("Browse output folder");
    output_layout->addWidget(browse_output_button_);
    output_layout->addWidget(output_dir_field_);

    io_layout->addLayout(output_layout);
    io_layout->addStretch();

    return io_group;
}

QGroupBox *Window::create_settings_group() {
    auto settings_group = new QGroupBox();
    settings_group->setFlat(true);
    options_.settings_layout = new QFormLayout(settings_group);
    options_.settings_layout->setContentsMargins(0, 10, 0, 0);
    options_.settings_layout->setFieldGrowthPolicy(
        QFormLayout::FieldsStayAtSizeHint
    );
    options_.settings_layout->setLabelAlignment(
        Qt::AlignRight | Qt::AlignVCenter
    );
    auto style = this->style();

    // Preprocessing
    add_display_presets_widget();
#if defined(PDF_ENABLED)
    add_pdf_pixel_density_widget(style, &options_);
#endif
    options_.settings_layout->addItem(new QSpacerItem(0, 25));
    add_convert_to_greyscale_widget(style, &options_);
    add_contrast_widget(style, &options_);
    options_.settings_layout->addItem(new QSpacerItem(0, 25));
    add_double_page_spread_widget(style, &options_);
    add_remove_spine_widget(style, &options_);

    options_.settings_layout->addItem(new QSpacerItem(0, 25));
    options_.advanced_options_check_box = new QCheckBox();
    auto advanced_options_label = new QLabel("Show advanced options");
    QFont font = advanced_options_label->font();
    font.setBold(true);
    advanced_options_label->setFont(font);
    auto container = new QWidget();
    auto container_layout = new QHBoxLayout(container);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->addWidget(options_.advanced_options_check_box);
    container_layout->addWidget(advanced_options_label);
    container_layout->addStretch();
    options_.settings_layout->addRow(container);

    // Colour
    add_linear_light_resampling_widget(style, &options_);
    add_quantization_widgets(style, &options_);
    add_scaling_widgets(style, &options_);

    options_.settings_layout->addItem(new QSpacerItem(0, 25));
    add_image_format_widgets(style, &options_);
    add_parallel_workers_widget(style, &options_);

    on_advanced_options_changed(
        options_.advanced_options_check_box->checkState()
    );

    return settings_group;
}

void Window::create_log_group() {
    log_group_ = new QGroupBox("Total progress");
    log_group_->setVisible(false);
    log_group_->setFlat(true);
    auto log_layout = new QVBoxLayout(log_group_);

    auto time_layout = new QHBoxLayout();
    elapsed_label_ = new QLabel("Elapsed: –");
    eta_label_ = new QLabel("ETA: –");
    time_layout->addWidget(elapsed_label_);
    time_layout->addWidget(eta_label_);
    time_layout->addStretch();
    time_layout->setSpacing(50);

    progress_bar_ = new QProgressBar();
    progress_bar_->setVisible(false);
    progress_bar_->setTextVisible(true);
    progress_bar_->setFormat("%p % (%v / %m pages)");

    log_output_ = new QTextEdit();
    log_output_->setVisible(false);
    log_output_->setReadOnly(true);

    log_layout->addWidget(progress_bar_);
    log_layout->addLayout(time_layout);
    log_layout->addWidget(log_output_);
}

void Window::add_display_presets_widget() {
    auto display_preset_brand = QString::fromStdString(display_preset.brand);
    options_.display_preset_button = new QPushButton(display_preset_brand);
    auto display_menu = new QMenu(this);

    if (auto custom_action = display_menu->addAction("None")) {
        connect(custom_action, &QAction::triggered, this, [this]() {
            set_display_preset("None", "");
        });
    }
    display_menu->addSeparator();

    for (const auto &[brand, model] : DISPLAY_PRESETS) {
        if (!model.has_value()) {
            continue;
        }

        auto brand_qstr = QString::fromStdString(brand);
        auto brand_menu = display_menu->addMenu(brand_qstr);
        if (!brand_menu) {
            continue;
        }

        for (const auto &[model_name, _] : *model) {
            auto model_name_qstr = QString::fromStdString(model_name);
            auto model_action = brand_menu->addAction(model_name_qstr);
            if (!model_action) {
                continue;
            }

            connect(
                model_action,
                &QAction::triggered,
                this,
                [this, brand, model_name]() {
                    set_display_preset(brand, model_name);
                }
            );
        }
    }

    auto label
        = create_widget_with_info(style(), new QLabel("Device preset"), "");
    options_.display_preset_button->setMenu(display_menu);

    auto output_format_label = new QLabel("Output format");
    options_.output_format_combo_box
        = create_combo_box({"EPUB", "CBZ"}, "EPUB");
    auto output_format_container = create_control_with_info(
        style(), options_.output_format_combo_box, OUTPUT_FORMAT_TOOLTIP
    );

    options_.settings_layout->addRow(label, options_.display_preset_button);
    options_.settings_layout->addRow(
        output_format_label, output_format_container
    );
}

void Window::start_next_task() {
    if (task_queue_.isEmpty()
        || running_processes_.size() >= max_concurrent_workers_
        || is_processing_cancelled_) {
        return;
    }

    PageTask task = task_queue_.dequeue();
    QString source_qstr = QString::fromStdString(task.source_file.string());

    auto &job = jobs_[source_qstr];
    if (job.progress_bar == nullptr) {
        progress_bars_group_->setVisible(true);

        auto widget = new QWidget();
        auto vbox = new QVBoxLayout(widget);
        vbox->setContentsMargins(5, 2, 5, 2);

        auto progress_layout = new QHBoxLayout();
        auto filename = QFileInfo(source_qstr).completeBaseName() + ".cbz";
        auto label = new QLabel("<code>" + filename + "</code>");
        auto progress_bar = new QProgressBar();
        progress_bar->setMaximum(job.tasks_remaining);
        progress_bar->setValue(0);
        progress_bar->setTextVisible(true);
        progress_bar->setFormat("%p % (%v / %m pages)");

        progress_layout->addWidget(label);
        progress_layout->addWidget(progress_bar);

        auto time_layout = new QHBoxLayout();
        auto elapsed_label = new QLabel("Elapsed: –");
        auto eta_label = new QLabel("ETA: –");
        time_layout->addWidget(elapsed_label);
        time_layout->addWidget(eta_label);
        time_layout->addStretch();
        time_layout->setSpacing(50);

        vbox->addLayout(progress_layout);
        vbox->addLayout(time_layout);

        progress_bars_layout_->addWidget(widget);
        job.widget = widget;
        job.progress_bar = progress_bar;
        job.elapsed_label = elapsed_label;
        job.eta_label = eta_label;

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()
        )
                      .count();
        FileTimer file_timer;
        file_timer.start_time = ms;
        file_timer.last_eta_time = ms;
        file_timer.images_since_last_eta = 0;
        job.timer = file_timer;
    }

    auto process = new QProcess(this);
    running_processes_.append(process);
    running_tasks_.insert(process, task);

    connect(
        process,
        &QProcess::readyReadStandardOutput,
        this,
        &Window::on_worker_output
    );
    connect(
        process,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        &Window::on_worker_finished
    );

    QString program = QCoreApplication::applicationFilePath();
    QStringList arguments;
    arguments << "-source_file"
              << QString::fromStdString(task.source_file.string())
              << "-output_dir"
              << QString::fromStdString(task.output_dir.string())
              << "-output_base_name"
              << QString::fromStdString(task.output_base_name) << "-page_number"
              << QString::number(task.page_number) << "-path_in_archive"
              << QString::fromStdString(
                     task.path_in_archive
                 ) // Empty string if not set
#if defined(PDF_ENABLED)
              << "-pdf_pixel_density" << QString::number(task.pdf_pixel_density)
#endif
              << "-convert_pages_to_greyscale"
              << (task.convert_pages_to_greyscale ? "1" : "0")
              << "-double_page_spread_actions"
              << QString::number(task.double_page_spread_action)
              << "-rotation_direction"
              << QString::number(task.rotation_direction)
              << "-linear_light_resampling"
              << QString::number(task.linear_light_resampling)
              << "-remove_spine" << QString::number(task.remove_spine)
              << "-stretch_page_contrast"
              << (task.stretch_page_contrast ? "1" : "0") << "-scale_pages"
              << (task.scale_pages ? "1" : "0") << "-page_width"
              << QString::number(task.page_width) << "-page_height"
              << QString::number(task.page_height) << "-page_resampler"
              << QString::number(static_cast<int>(task.page_resampler))
              << "-quantize_pages" << (task.quantize_pages ? "1" : "0")
              << "-bit_depth" << QString::number(task.bit_depth) << "-dither"
              << QString::number(task.dither) << "-image_format"
              << QString::fromStdString(task.image_format) << "-is_lossy"
              << (task.is_lossy ? "1" : "0") << "-quality_type_is_distance"
              << (task.quality_type_is_distance ? "1" : "0") << "-quality"
              << QString::number(task.quality) << "-compression_effort"
              << QString::number(task.compression_effort);

    process->start(program, arguments);
}

void Window::handle_log_message(const QString &message) {
    log_output_->setVisible(true);
    log_output_->append(message);
}

void Window::handle_task_finished() {
    pages_processed_ += 1;
    images_since_last_eta_ += 1;
    progress_bar_->setValue(pages_processed_);
}

PageTask Window::create_task(
    const fs::path &source_file, fs::path output_dir, int page_num
) {
    PageTask task;
    auto source_qstr = QString::fromStdString(source_file.string());

    task.source_file = source_file;
    task.output_dir = std::move(output_dir);
    task.page_number = page_num;

    auto total_pages = jobs_.value(source_qstr).pages_total;
    auto padding_width
        = total_pages > 0
            ? static_cast<int>(std::floor(std::log10(total_pages))) + 1
            : 1;
    task.output_base_name
        = QString("%1")
              .arg(page_num + 1, padding_width, 10, QChar('0'))
              .toStdString();

#if defined(PDF_ENABLED)
    task.pdf_pixel_density = options_.pdf_pixel_density_spin_box->value();
#endif
    task.convert_pages_to_greyscale
        = options_.convert_to_greyscale->isChecked();
    task.double_page_spread_action
        = (DoublePageSpreadActions)
              options_.double_page_spread_combo_box->currentIndex();
    if (options_.rotation_direction_combo_box->currentText() == "Clockwise") {
        task.rotation_direction = CLOCKWISE;
    }
    else {
        task.rotation_direction = COUNTERCLOCKWISE;
    }
    task.remove_spine = options_.remove_spine_check_box->isChecked();
    task.linear_light_resampling
        = options_.linear_light_resampling_check_box->isChecked();
    task.stretch_page_contrast = options_.contrast_check_box->isChecked();
    task.scale_pages = options_.enable_image_scaling_check_box->isChecked();
    task.page_width = options_.width_spin_box->value();
    task.page_height = options_.height_spin_box->value();
    auto resampler = options_.resampler_combo_box->currentText();
    if (resampler == "Bicubic interpolation") {
        task.page_resampler = VIPS_KERNEL_CUBIC;
    }
    else if (resampler == "Bilinear interpolation") {
        task.page_resampler = VIPS_KERNEL_LINEAR;
    }
    else if (resampler == "Lanczos 2") {
        task.page_resampler = VIPS_KERNEL_LANCZOS2;
    }
    else if (resampler == "Lanczos 3") {
        task.page_resampler = VIPS_KERNEL_LANCZOS3;
    }
    else if (resampler == "Magic Kernel Sharp 2013") {
        task.page_resampler = VIPS_KERNEL_MKS2013;
    }
    else if (resampler == "Magic Kernel Sharp 2021") {
        task.page_resampler = VIPS_KERNEL_MKS2021;
    }
    else if (resampler == "Mitchell") {
        task.page_resampler = VIPS_KERNEL_MITCHELL;
    }
    else {
        task.page_resampler = VIPS_KERNEL_NEAREST;
    }
    task.quantize_pages
        = options_.enable_image_quantization_check_box->isChecked();
    task.bit_depth = 1 << options_.bit_depth_combo_box->currentIndex();
    task.dither = options_.dithering_spin_box->value();
    task.image_format
        = options_.image_format_combo_box->currentText().toStdString();
    task.is_lossy
        = options_.image_compression_type_combo_box->currentText() == "Lossy";
    task.quality_type_is_distance
        = options_.image_quality_label_jpeg_xl->currentText() == "Distance";
    task.quality = options_.image_quality_spin_box->value();
    task.compression_effort = options_.image_compression_spin_box->value();
    return task;
}

void Window::update_file_list_buttons() {
    auto count = file_list_->count();
    auto has_items = count > 0;

    start_button_->setEnabled(has_items);
    remove_selected_button_->setVisible(has_items);
    clear_all_button_->setVisible(has_items);

#if defined(PDF_ENABLED)
    auto pdf_inputs_exist = false;
    for (auto i = 0; i < count; i += 1) {
        auto path_variant = file_list_->item(i)->data(Qt::UserRole);
        auto path = fs::path(path_variant.toString().toStdString());
        auto extension = path.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(), ::tolower
        );
        if (extension == ".pdf") {
            pdf_inputs_exist = true;
            break;
        }
    }

    options_.pdf_pixel_density_label->setVisible(pdf_inputs_exist);
    options_.pdf_pixel_density_combo_box->setVisible(pdf_inputs_exist);
    options_.pdf_pixel_density_tooltip->setVisible(pdf_inputs_exist);
    options_.pdf_options_container->setVisible(pdf_inputs_exist);
#endif

    if (!has_items) {
        remove_selected_button_->setEnabled(false);
        return;
    }

    auto has_selection = !file_list_->selectedItems().isEmpty();
    remove_selected_button_->setEnabled(has_selection);
}

void Window::set_display_preset(
    const std::string &brand, const std::string &model
) {
    display_preset.brand = brand;
    display_preset.model = model;
    QString text = model.empty() ? QString::fromStdString(brand)
                                 : QString::fromStdString(brand + " " + model);
    options_.display_preset_button->setText(text);
    on_display_preset_changed();
}

void Window::create_archive(const QString &source_archive_path) {
    auto source_path = fs::path(source_archive_path.toStdString());
    auto title = source_path.stem();

    auto temp_dir = fs::path(temp_base_dir_) / title;
    auto output_filename = source_path.filename();

    QCoreApplication::processEvents();

    try {
        if (options_.output_format_combo_box->currentText() == "EPUB") {
            output_filename = output_filename.replace_extension(".epub");
            auto final_output_path = output_path_ / output_filename;
            create_epub(temp_dir, final_output_path, title.generic_string());
        }
        else {
            output_filename = output_filename.replace_extension(".cbz");
            auto final_output_path = output_path_ / output_filename;
            create_cbz(temp_dir, final_output_path);
        }
    }
    catch (const NoImagesError &) {
        log_output_->append(
            QString("Error: source '%1' does not contain any images.")
                .arg(source_archive_path)
        );
        log_output_->setVisible(true);
        return;
    }
    catch (const ArchiveError &) {
        log_output_->append("Error: failed to create archive.");
        log_output_->setVisible(true);
        return;
    }
    catch (const std::exception &e) {
        log_output_->append(
            QString("Error: %1").arg(QString::fromUtf8(e.what()))
        );
        log_output_->setVisible(true);
        return;
    }

    try {
        fs::remove_all(temp_dir);
    }
    catch (const std::exception &e) {
        log_output_->append(
            QString("Error cleaning up temp directory %1: %2")
                .arg(QString::fromStdString(temp_dir.string()), e.what())
        );
        log_output_->setVisible(true);
    }
}

Window::~Window() {
}
