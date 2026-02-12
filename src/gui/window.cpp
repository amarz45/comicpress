#include "include/window.hpp"
#include "../include/task.hpp"
#include "include/display_presets.hpp"
#include "include/options.hpp"
#include "include/window_util.hpp"
#include "qboxlayout.h"
#include "qnamespace.h"
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

Window::Window(QWidget *parent) : QMainWindow(parent), eta_recent_intervals(5) {
    // Timer
    this->timer = new QTimer(this);
    connect(this->timer, &QTimer::timeout, this, &Window::update_time_labels);
    this->start_time = std::nullopt;
    this->last_eta_recent_time = std::nullopt;
    this->images_since_last_eta_recent = 0;
    this->last_progress_value = 0.0;
    this->is_processing_cancelled = false;
    this->is_programmatically_changing_values = false;
    this->temp_base_dir = "";

    this->setWindowTitle("Comicpress");
    this->central_widget = new QWidget(this);
    this->setCentralWidget(central_widget);

    this->setup_ui();
    this->set_display_preset("None", "");
    this->on_enable_image_scaling_changed(
        this->options.enable_image_scaling_check_box->checkState()
    );
    this->on_double_page_spread_changed(
        this->options.double_page_spread_combo_box->currentText()
    );
    this->connect_signals();
}

void Window::setup_ui() {
    auto container_layout = new QHBoxLayout(this->central_widget);
    container_layout->setContentsMargins(40, 20, 40, 20);
    auto content_widget = new QWidget();
    container_layout->setAlignment(Qt::AlignTop);
    content_widget->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Preferred
    );

    auto io_group = this->create_io_group();
    this->options.settings_group = this->create_settings_group();

    this->progress_bars_group = new QGroupBox("File progress");
    this->progress_bars_group->setFlat(true);
    this->progress_bars_layout = new QVBoxLayout(this->progress_bars_group);
    this->progress_bars_group->setVisible(false);

    this->create_log_group();

    auto tabs = new QTabWidget();
    tabs->setDocumentMode(true);

    auto io_scroll = new QScrollArea();
    io_scroll->setWidget(io_group);
    io_scroll->setWidgetResizable(true);
    io_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    io_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabs->addTab(io_scroll, "Files");

    auto settings_scroll = new QScrollArea();
    settings_scroll->setWidget(this->options.settings_group);
    settings_scroll->setWidgetResizable(true);
    settings_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    settings_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabs->addTab(settings_scroll, "Options");

    auto action_group = new QWidget();
    auto action_layout = new QHBoxLayout(action_group);
    this->start_button = new QPushButton("Start");
    this->cancel_button = new QPushButton("Cancel");
    this->start_button->setEnabled(false);
    this->cancel_button->setEnabled(false);
    action_layout->addStretch();
    action_layout->addWidget(this->start_button);
    action_layout->addWidget(this->cancel_button);

    this->main_layout = new QVBoxLayout(content_widget);
    this->main_layout->setContentsMargins(0, 0, 0, 0);

    this->main_layout->addWidget(tabs);
    this->main_layout->addSpacing(20);

    auto separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    this->main_layout->addWidget(separator);

    this->main_layout->addWidget(this->progress_bars_group);
    this->main_layout->addWidget(log_group);
    this->main_layout->addItem(
        new QSpacerItem(1000, 0, QSizePolicy::Preferred, QSizePolicy::Fixed)
    );
    this->main_layout->addWidget(action_group);

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

    this->file_list = new QListWidget();
    this->file_list->setFont(QFont("monospace"));
    this->file_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->file_list->setMaximumHeight(500);

    this->add_files_button = new QPushButton("Add input files");
    this->remove_selected_button = new QPushButton("Remove selected");
    this->clear_all_button = new QPushButton("Remove all");

    this->remove_selected_button->setVisible(false);
    this->clear_all_button->setVisible(false);

    file_buttons_layout->addWidget(this->add_files_button);
    file_buttons_layout->addWidget(this->remove_selected_button);
    file_buttons_layout->addWidget(this->clear_all_button);
    file_buttons_layout->addStretch();

    io_layout->addLayout(file_buttons_layout);
    io_layout->addWidget(file_list, 1);

    auto output_layout = new QHBoxLayout();

    auto documents_dir = QDir(
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );
    this->output_dir_field
        = new QLineEdit(documents_dir.filePath("Comicpress"));
    this->browse_output_button = new QPushButton("Browse output folder");
    output_layout->addWidget(this->browse_output_button);
    output_layout->addWidget(this->output_dir_field);

    io_layout->addLayout(output_layout);
    io_layout->addStretch();

    return io_group;
}

QGroupBox *Window::create_settings_group() {
    auto settings_group = new QGroupBox();
    settings_group->setFlat(true);
    this->options.settings_layout = new QFormLayout(settings_group);
    this->options.settings_layout->setContentsMargins(0, 10, 0, 0);
    this->options.settings_layout->setFieldGrowthPolicy(
        QFormLayout::FieldsStayAtSizeHint
    );
    this->options.settings_layout->setLabelAlignment(
        Qt::AlignRight | Qt::AlignVCenter
    );
    auto style = this->style();

    // Preprocessing
    this->add_display_presets_widget();
#if defined(PDFIUM_ENABLED)
    add_pdf_pixel_density_widget(style, &this->options);
#endif
    this->options.settings_layout->addItem(new QSpacerItem(0, 25));
    add_convert_to_greyscale_widget(style, &this->options);
    add_contrast_widget(style, &this->options);
    this->options.settings_layout->addItem(new QSpacerItem(0, 25));
    add_double_page_spread_widget(style, &this->options);
    add_remove_spine_widget(style, &this->options);

    this->options.settings_layout->addItem(new QSpacerItem(0, 25));
    this->options.advanced_options_check_box = new QCheckBox();
    auto advanced_options_label = new QLabel("Show advanced options");
    QFont font = advanced_options_label->font();
    font.setBold(true);
    advanced_options_label->setFont(font);
    auto container = new QWidget();
    auto container_layout = new QHBoxLayout(container);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->addWidget(this->options.advanced_options_check_box);
    container_layout->addWidget(advanced_options_label);
    container_layout->addStretch();
    this->options.settings_layout->addRow(container);

    // Colour
    add_linear_light_resampling_widget(style, &this->options);
    add_quantization_widgets(style, &this->options);
    add_scaling_widgets(style, &this->options);

    this->options.settings_layout->addItem(new QSpacerItem(0, 25));
    add_image_format_widgets(style, &this->options);
    add_parallel_workers_widget(style, &this->options);

    this->on_advanced_options_changed(
        this->options.advanced_options_check_box->checkState()
    );

    return settings_group;
}

void Window::create_log_group() {
    log_group = new QGroupBox("Total progress");
    log_group->setVisible(false);
    log_group->setFlat(true);
    auto log_layout = new QVBoxLayout(log_group);

    auto time_layout = new QHBoxLayout();
    this->elapsed_label = new QLabel("Elapsed: –");
    this->eta_overall_label = new QLabel("ETA (overall): –");
    this->eta_recent_label = new QLabel("ETA (recent): –");
    time_layout->addWidget(this->elapsed_label);
    time_layout->addWidget(this->eta_overall_label);
    time_layout->addWidget(this->eta_recent_label);
    time_layout->addStretch();
    time_layout->setSpacing(50);

    this->progress_bar = new QProgressBar();
    this->progress_bar->setVisible(false);
    this->progress_bar->setTextVisible(true);
    this->progress_bar->setFormat("%p % (%v / %m pages)");

    this->log_output = new QTextEdit();
    this->log_output->setVisible(false);
    this->log_output->setReadOnly(true);

    log_layout->addWidget(this->progress_bar);
    log_layout->addLayout(time_layout);
    log_layout->addWidget(this->log_output);
}

void Window::add_display_presets_widget() {
    auto display_preset = QString::fromStdString(this->display_preset.brand);
    this->options.display_preset_button = new QPushButton(display_preset);
    auto display_menu = new QMenu(this);

    if (auto custom_action = display_menu->addAction("None")) {
        connect(custom_action, &QAction::triggered, this, [this]() {
            this->set_display_preset("None", "");
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
                    this->set_display_preset(brand, model_name);
                }
            );
        }
    }

    auto label = create_widget_with_info(
        this->style(), new QLabel("Device preset"), ""
    );
    this->options.display_preset_button->setMenu(display_menu);

    this->options.settings_layout->addRow(
        label, this->options.display_preset_button
    );
}

void Window::start_next_task() {
    if (task_queue.isEmpty()
        || running_processes.size() >= max_concurrent_workers
        || is_processing_cancelled) {
        return;
    }

    PageTask task = task_queue.dequeue();
    QString source_qstr = QString::fromStdString(task.source_file.string());

    if (!this->active_progress_bars.contains(source_qstr)) {
        this->progress_bars_group->setVisible(true);

        auto widget = new QWidget();
        auto vbox = new QVBoxLayout(widget);
        vbox->setContentsMargins(5, 2, 5, 2);

        auto progress_layout = new QHBoxLayout();
        auto filename = QFileInfo(source_qstr).completeBaseName() + ".cbz";
        auto label = new QLabel("<code>" + filename + "</code>");
        auto progressBar = new QProgressBar();
        progressBar->setMaximum(this->archive_task_counts.value(source_qstr));
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p % (%v / %m pages)");

        progress_layout->addWidget(label);
        progress_layout->addWidget(progressBar);

        auto time_layout = new QHBoxLayout();
        auto elapsed_label = new QLabel("Elapsed: –");
        auto eta_overall_label = new QLabel("ETA (overall): –");
        auto eta_recent_label = new QLabel("ETA (recent): –");
        time_layout->addWidget(elapsed_label);
        time_layout->addWidget(eta_overall_label);
        time_layout->addWidget(eta_recent_label);
        time_layout->addStretch();
        time_layout->setSpacing(50);

        vbox->addLayout(progress_layout);
        vbox->addLayout(time_layout);

        this->progress_bars_layout->addWidget(widget);
        this->active_file_widgets.insert(source_qstr, widget);
        this->active_progress_bars.insert(source_qstr, progressBar);
        this->file_elapsed_labels.insert(source_qstr, elapsed_label);
        this->file_eta_overall_labels.insert(source_qstr, eta_overall_label);
        this->file_eta_recent_labels.insert(source_qstr, eta_recent_label);

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()
        )
                      .count();
        FileTimer file_timer;
        file_timer.start_time = ms;
        file_timer.last_eta_recent_time = ms;
        file_timer.images_since_last_eta_recent = 0;
        this->file_timers.insert(source_qstr, file_timer);
    }

    QProcess *process = new QProcess(this);
    running_processes.append(process);
    running_tasks.insert(process, task);

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
#if defined(PDFIUM_ENABLED)
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
    log_output->setVisible(true);
    log_output->append(message);
}

void Window::handle_task_finished() {
    pages_processed++;
    images_since_last_eta_recent++;
    this->progress_bar->setValue(pages_processed);
}

PageTask
Window::create_task(fs::path source_file, fs::path output_dir, int page_num) {
    PageTask task;
    auto source_qstr = QString::fromStdString(source_file.string());

    task.source_file = source_file;
    task.output_dir = output_dir;
    task.page_number = page_num;

    auto total_pages = this->total_pages_per_archive.value(source_qstr, 0);
    auto padding_width
        = total_pages > 0
            ? static_cast<int>(std::floor(std::log10(total_pages))) + 1
            : 1;
    task.output_base_name
        = QString("%1")
              .arg(page_num + 1, padding_width, 10, QChar('0'))
              .toStdString();

#if defined(PDFIUM_ENABLED)
    task.pdf_pixel_density = this->options.pdf_pixel_density_spin_box->value();
#endif
    task.convert_pages_to_greyscale
        = this->options.convert_to_greyscale->isChecked();
    task.double_page_spread_action
        = (DoublePageSpreadActions)this->options.double_page_spread_combo_box
              ->currentIndex();
    if (this->options.rotation_direction_combo_box->currentText()
        == "Clockwise") {
        task.rotation_direction = CLOCKWISE;
    }
    else {
        task.rotation_direction = COUNTERCLOCKWISE;
    }
    task.remove_spine = this->options.remove_spine_check_box->isChecked();
    task.linear_light_resampling
        = this->options.linear_light_resampling_check_box->isChecked();
    task.stretch_page_contrast = this->options.contrast_check_box->isChecked();
    task.scale_pages
        = this->options.enable_image_scaling_check_box->isChecked();
    task.page_width = this->options.width_spin_box->value();
    task.page_height = this->options.height_spin_box->value();
    auto resampler = this->options.resampler_combo_box->currentText();
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
        = this->options.enable_image_quantization_check_box->isChecked();
    task.bit_depth
        = std::pow(2, this->options.bit_depth_combo_box->currentIndex());
    task.dither = this->options.dithering_spin_box->value();
    task.image_format
        = this->options.image_format_combo_box->currentText().toStdString();
    task.is_lossy
        = this->options.image_compression_type_combo_box->currentText()
       == "Lossy";
    task.quality_type_is_distance
        = this->options.image_quality_label_jpeg_xl->currentText()
       == "Distance";
    task.quality = this->options.image_quality_spin_box->value();
    task.compression_effort = this->options.image_compression_spin_box->value();
    return task;
}

void Window::update_file_list_buttons() {
    auto count = this->file_list->count();
    auto has_items = count > 0;

    this->start_button->setEnabled(has_items);
    this->remove_selected_button->setVisible(has_items);
    this->clear_all_button->setVisible(has_items);

#if defined(PDFIUM_ENABLED)
    auto pdf_inputs_exist = false;
    for (auto i = 0; i < count; i += 1) {
        auto path_variant = this->file_list->item(i)->data(Qt::UserRole);
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

    this->options.pdf_pixel_density_label->setVisible(pdf_inputs_exist);
    this->options.pdf_pixel_density_combo_box->setVisible(pdf_inputs_exist);
    this->options.pdf_pixel_density_tooltip->setVisible(pdf_inputs_exist);
    this->options.pdf_options_container->setVisible(pdf_inputs_exist);
#endif

    if (!has_items) {
        this->remove_selected_button->setEnabled(false);
        return;
    }

    auto has_selection = !this->file_list->selectedItems().isEmpty();
    this->remove_selected_button->setEnabled(has_selection);
}

void Window::set_display_preset(std::string brand, std::string model) {
    this->display_preset.brand = brand;
    this->display_preset.model = model;
    QString text = model.empty() ? QString::fromStdString(brand)
                                 : QString::fromStdString(brand + " " + model);
    this->options.display_preset_button->setText(text);
    this->on_display_preset_changed();
}

void Window::create_archive(const QString &source_archive_path) {
    auto source_path = fs::path(source_archive_path.toStdString());
    auto temp_dir = fs::path(this->temp_base_dir) / source_path.stem();
    auto final_filename = source_path.filename().replace_extension(".cbz");
    auto final_output_path = output_path / final_filename;

    QCoreApplication::processEvents();

    auto a = archive_write_new();
    archive_write_set_format_zip(a);
    archive_write_set_options(a, "compression-level=0");
    archive_write_open_filename(a, final_output_path.string().c_str());

    try {
        for (const auto &dir_entry :
             fs::recursive_directory_iterator(temp_dir)) {
            if (dir_entry.is_regular_file()) {
                const fs::path &path = dir_entry.path();
                fs::path relative_path = fs::relative(path, temp_dir);

                struct archive_entry *entry = archive_entry_new();
                archive_entry_set_pathname(
                    entry, relative_path.string().c_str()
                );
                archive_entry_set_size(entry, fs::file_size(path));
                archive_entry_set_filetype(entry, AE_IFREG);
                archive_entry_set_perm(entry, 0644);
                archive_write_header(a, entry);

                std::ifstream file_stream(path, std::ios::binary);
                char buffer[8192];
                while (file_stream.good()) {
                    file_stream.read(buffer, sizeof(buffer));
                    archive_write_data(
                        a, buffer, static_cast<size_t>(file_stream.gcount())
                    );
                }

                archive_entry_free(entry);
            }
        }
    }
    catch (const std::exception &e) {
        log_output->setVisible(true);
        log_output->append(QString("Error during archiving: %1").arg(e.what()));
    }

    archive_write_close(a);
    archive_write_free(a);

    try {
        fs::remove_all(temp_dir);
    }
    catch (const std::exception &e) {
        log_output->setVisible(true);
        log_output->append(
            QString("Error cleaning up temp directory %1: %2")
                .arg(QString::fromStdString(temp_dir.string()), e.what())
        );
    }
}

Window::~Window() {
}
