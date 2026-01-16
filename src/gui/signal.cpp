#include "include/display_presets.hpp"
#include "include/window.hpp"
#include <QCoreApplication>
#include <QFileDialog>
#include <archive.h>
#include <archive_entry.h>
#if defined(PDFIUM_ENABLED)
#include <fpdfview.h>
#endif
#include <stdexcept>

void Window::connect_signals() {
    connect(
        this->add_files_button,
        &QPushButton::clicked,
        this,
        &Window::on_add_files_clicked
    );
    connect(
        this->remove_selected_button,
        &QPushButton::clicked,
        this,
        &Window::on_remove_selected_clicked
    );
    connect(
        this->clear_all_button,
        &QPushButton::clicked,
        this,
        &Window::on_clear_all_clicked
    );
    connect(
        this->browse_output_button,
        &QPushButton::clicked,
        this,
        &Window::on_browse_output_clicked
    );
    connect(
        this->options.pdf_pixel_density_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_pdf_pixel_density_combo_box_changed
    );
    connect(
        this->options.double_page_spread_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_double_page_spread_changed
    );
    connect(
        this->options.convert_to_greyscale,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_preset_option_modified
    );
    connect(
        this->options.bit_depth_combo_box,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        this->options.width_spin_box,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        this->options.height_spin_box,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        this->options.enable_image_scaling_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_preset_option_modified
    );
    connect(
        this->options.advanced_options_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_advanced_options_changed
    );
    connect(
        this->options.enable_image_scaling_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_enable_image_scaling_changed
    );
    connect(
        this->options.enable_image_quantization_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_enable_image_quantization_changed
    );
    connect(
        this->options.image_format_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_image_format_changed
    );
    connect(
        this->options.image_compression_spin_box,
        &QSpinBox::valueChanged,
        this,
        &Window::on_image_compression_changed
    );
    connect(
        this->options.image_compression_type_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_image_compression_type_changed_explicit
    );
    connect(
        this->options.image_quality_spin_box,
        &QDoubleSpinBox::valueChanged,
        this,
        &Window::on_image_quality_changed
    );
    connect(
        this->options.image_quality_label_jpeg_xl,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_jpeg_xl_quality_type_changed
    );
    connect(
        this->start_button,
        &QPushButton::clicked,
        this,
        &Window::on_start_button_clicked
    );
    connect(
        this->cancel_button,
        &QPushButton::clicked,
        this,
        &Window::on_cancel_button_clicked
    );
    connect(
        this->file_list,
        &QListWidget::itemSelectionChanged,
        this,
        &Window::update_file_list_buttons
    );
}

void Window::on_add_files_clicked() {
    auto files = QFileDialog::getOpenFileNames(
        this,
        "Select input files",
        "",
#if defined(PDFIUM_ENABLED)
        "Supported files (*.pdf *.cbz *.cbr)"
#else
        "Supported files (*.cbz *.cbr)"
#endif
    );

    if (files.isEmpty()) {
        return;
    }

    QStringList existing_paths;
    for (int i = 0; i < file_list->count(); i += 1) {
        if (auto item = file_list->item(i)) {
            if (auto data = item->data(Qt::UserRole); data.isValid()) {
                existing_paths.append(data.toString());
            }
        }
    }

    for (const QString &file : files) {
        if (!existing_paths.contains(file)) {
            auto item = new QListWidgetItem(QFileInfo(file).fileName());
            item->setData(Qt::UserRole, file);
            file_list->addItem(item);
        }
    }

    this->update_file_list_buttons();
}

void Window::on_remove_selected_clicked() {
    qDeleteAll(this->file_list->selectedItems());
    this->update_file_list_buttons();
}

void Window::on_clear_all_clicked() {
    this->file_list->clear();
    this->update_file_list_buttons();
}

void Window::on_browse_output_clicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select output folder"),
        this->output_dir_field->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        this->output_dir_field->setText(dir);
    }
}

void Window::on_pdf_pixel_density_combo_box_changed(const QString &text) {
    auto spin = this->options.pdf_pixel_density_spin_box;
    if (text == "Custom") {
        spin->setVisible(true);
        return;
    }

    spin->setVisible(false);
    if (text == "Standard (300\u202fPPI, fast)") {
        spin->setValue(300);
    }
    else if (text == "High (600\u202fPPI)") {
        spin->setValue(600);
    }
    else if (text == "Ultra (1200\u202fPPI, recommended)") {
        spin->setValue(1200);
    }
}

void Window::on_double_page_spread_changed(const QString &text) {
    bool should_show = (text == "Rotate page" || text == "Rotate and split");
    this->options.rotation_options_container->setVisible(should_show);
}

void Window::on_preset_option_modified() {
    if (this->is_programmatically_changing_values) {
        return;
    }

    if (this->display_preset.brand != "None") {
        this->set_display_preset("None", "");
    }
}

void Window::on_display_preset_changed() {
    auto brand = this->display_preset.brand;
    auto model = this->display_preset.model;
    auto is_custom = brand == "None";

    if (is_custom) {
        return;
    }

    this->is_programmatically_changing_values = true;

    const auto &models = DISPLAY_PRESETS.at(brand).value();
    auto it = std::find_if(
        models.begin(), models.end(), [&model](const auto &pair) {
            return pair.first == model;
        }
    );

    if (it != models.end()) {
        const Display &display = it->second;
        this->options.enable_image_scaling_check_box->setChecked(true);
        this->options.width_spin_box->setValue(display.width);
        this->options.height_spin_box->setValue(display.height);
        this->options.bit_depth_combo_box->setCurrentIndex(
            display.bit_depth_index
        );
        this->options.convert_to_greyscale->setChecked(!display.colour);
    }

    this->is_programmatically_changing_values = false;
}

void Window::on_advanced_options_changed(int state) {
    bool is_checked = state == Qt::Checked;

    this->options.linear_light_resampling_label->setVisible(is_checked);
    this->options.linear_light_resampling_container->setVisible(is_checked);

    this->options.quantize_pages_label->setVisible(is_checked);
    this->options.quantize_pages_container->setVisible(is_checked);

    this->options.scale_pages_label->setVisible(is_checked);
    this->options.scale_pages_container->setVisible(is_checked);

    this->options.image_format_label->setVisible(is_checked);
    this->options.image_format_container->setVisible(is_checked);

    if (is_checked) {
        this->on_enable_image_quantization_changed(
            this->options.enable_image_quantization_check_box->checkState()
        );
        this->on_enable_image_scaling_changed(
            this->options.enable_image_scaling_check_box->checkState()
        );
        this->on_image_format_changed();
    }
    else {
        this->options.quantization_options_container->setVisible(false);
        this->options.scaling_options_container->setVisible(false);
        this->options.image_format_options_container->setVisible(false);
    }

    this->options.workers_label->setVisible(is_checked);
    this->options.workers_spin_box->setVisible(is_checked);
}

void Window::on_enable_image_scaling_changed(int state) {
    bool is_checked = state == Qt::Checked;
    this->options.scaling_options_container->setVisible(is_checked);
}

void Window::on_enable_image_quantization_changed(int state) {
    bool is_checked = state == Qt::Checked;
    this->options.quantization_options_container->setVisible(is_checked);
    if (is_checked) {
        this->options.image_compression_type_combo_box->setCurrentText(
            "Lossless"
        );
    }
    else if (!this->compression_type_changed) {
        this->options.image_compression_type_combo_box->setCurrentText("Lossy");
    }
}

void Window::on_image_format_changed() {
    this->options.image_format_options_container->setVisible(true);
    auto img_format = this->options.image_format_combo_box->currentText();

    auto compression_min = 0;
    auto compression_max = 9;
    auto compression_effort = 0;
    QWidget *quality_label = this->options.image_quality_label_original;
    auto jxl_quality_label_visible = false;
    std::string quality_type = "Quality";
    double distance = 0.0;
    auto quality = 0;
    auto compression_type_visible = false;
    auto compression_effort_visible = true;

    if (img_format == "AVIF") {
        compression_effort = this->avif_compression_effort;
        quality = this->avif_quality;
        compression_type_visible = true;
    }
    else if (img_format == "JPEG") {
        quality = this->jpeg_quality;
        compression_effort_visible = false;
    }
    else if (img_format == "JPEG XL") {
        compression_min = 1;
        compression_effort = this->jpeg_xl_compression_effort;
        quality_label = this->options.image_quality_label_jpeg_xl;
        jxl_quality_label_visible = true;
        quality_type = this->options.image_quality_label_jpeg_xl->currentText()
                           .toStdString();
        distance = this->jpeg_xl_distance;
        quality = this->jpeg_xl_quality;
        compression_type_visible = true;
    }
    else if (img_format == "PNG") {
        compression_effort = this->png_compression_effort;
    }
    else if (img_format == "WebP") {
        compression_max = 6;
        compression_effort = this->webp_compression_effort;
        quality = this->webp_quality;
        compression_type_visible = true;
    }

    // Important: This should be before the others.
    this->on_jpeg_xl_quality_type_changed();

    this->options.image_compression_spin_box->setRange(
        compression_min, compression_max
    );
    this->options.image_compression_spin_box->setValue(compression_effort);
    this->options.image_quality_label = quality_label;
    this->options.image_quality_label_original->setVisible(
        !jxl_quality_label_visible
    );
    this->options.image_quality_label_jpeg_xl->setVisible(
        jxl_quality_label_visible
    );

    if (quality_type == "Quality") {
        this->options.image_quality_spin_box->setValue(quality);
    }
    else {
        this->options.image_quality_spin_box->setValue(distance);
    }

    this->options.image_compression_type_label->setVisible(
        compression_type_visible
    );
    this->options.image_compression_type_combo_box->setVisible(
        compression_type_visible
    );
    this->options.image_compression_type_tooltip->setVisible(
        compression_type_visible
    );
    this->options.image_compression_label->setVisible(
        compression_effort_visible
    );
    this->options.image_compression_spin_box->setVisible(
        compression_effort_visible
    );

    this->on_image_compression_type_changed(false);
}

void Window::on_image_compression_changed(int state) {
    auto img_format = this->options.image_format_combo_box->currentText();
    if (img_format == "AVIF") {
        this->avif_compression_effort = state;
    }
    else if (img_format == "JPEG XL") {
        this->jpeg_xl_compression_effort = state;
    }
    else if (img_format == "PNG") {
        this->png_compression_effort = state;
    }
    else if (img_format == "WebP") {
        this->webp_compression_effort = state;
    }
}

void Window::on_image_compression_type_changed_explicit() {
    this->on_image_compression_type_changed(true);
}

void Window::on_image_compression_type_changed(bool is_explicit) {
    auto img_format = this->options.image_format_combo_box->currentText();
    auto compression_type
        = this->options.image_compression_type_combo_box->currentText();
    auto image_quality_visible
        = img_format != "PNG"
       && (img_format == "JPEG" || compression_type == "Lossy");
    auto jpeg_xl_quality_tooltip_visible
        = img_format == "JPEG XL" && compression_type == "Lossy";

    this->options.image_quality_label->setVisible(image_quality_visible);
    this->options.image_quality_spin_box->setVisible(image_quality_visible);
    this->options.image_quality_jpeg_xl_tooltip->setVisible(
        jpeg_xl_quality_tooltip_visible
    );

    if (is_explicit) {
        this->compression_type_changed = true;
    }
}

void Window::on_image_quality_changed(int state) {
    auto img_format = this->options.image_format_combo_box->currentText();
    if (img_format == "AVIF") {
        this->avif_quality = state;
    }
    else if (img_format == "JPEG") {
        this->jpeg_quality = state;
    }
    else if (img_format == "JPEG XL") {
        if (this->options.image_quality_label_jpeg_xl->currentText()
            == "Distance") {
            this->jpeg_xl_distance = state;
        }
        else {
            this->jpeg_xl_quality = state;
        }
    }
    else if (img_format == "WebP") {
        this->webp_quality = state;
    }
}

void Window::on_jpeg_xl_quality_type_changed() {
    auto quality_type
        = this->options.image_quality_label_jpeg_xl->currentText();

    auto min = 0.0;
    auto max = 100.0;
    auto step = 1.0;
    auto decimals = 0;
    auto quality = this->jpeg_xl_quality;

    auto img_format = this->options.image_format_combo_box->currentText();
    if (img_format != "JPEG XL") {
        this->options.image_quality_spin_box->setRange(min, max);
        this->options.image_quality_spin_box->setSingleStep(step);
        this->options.image_quality_spin_box->setDecimals(decimals);
        return;
    }

    if (quality_type == "Distance") {
        max = 15.0;
        step = 0.1;
        decimals = 2;
        quality = this->jpeg_xl_distance;
    }

    this->options.image_quality_spin_box->setRange(min, max);
    this->options.image_quality_spin_box->setSingleStep(step);
    this->options.image_quality_spin_box->setDecimals(decimals);
    this->options.image_quality_spin_box->setValue(quality);
}

void Window::on_start_button_clicked() {
    QStringList input_file_paths;
    for (int i = 0; i < file_list->count(); i += 1) {
        input_file_paths.append(
            file_list->item(i)->data(Qt::UserRole).toString()
        );
    }

    if (input_file_paths.isEmpty()) {
        this->log_output->setVisible(true);
        log_output->append("No input files selected.");
        return;
    }

    // Create unique base temp directory for this run
    char temp_template[] = "/tmp/comicpress_XXXXXX";
    char *temp_base_cstr = mkdtemp(temp_template);
    if (!temp_base_cstr) {
        this->log_output->setVisible(true);
        log_output->append("Failed to create temporary directory.");
        return;
    }
    this->temp_base_dir = temp_base_cstr;

    this->options.settings_group->setEnabled(false);
    start_button->setEnabled(false);
    cancel_button->setEnabled(true);
    log_output->clear();
    task_queue.clear();
    running_processes.clear();
    running_tasks.clear();
    archive_task_counts.clear();
    this->total_pages_per_archive.clear();
    this->pages_processed_per_archive.clear();
    this->active_file_widgets.clear();
    this->active_progress_bars.clear();
    this->file_elapsed_labels.clear();
    this->file_eta_overall_labels.clear();
    this->file_eta_recent_labels.clear();
    this->file_timers.clear();
    is_processing_cancelled = false;
    pages_processed = 0;
    total_pages = 0;
    max_concurrent_workers = this->options.workers_spin_box->value();

    this->progress_bar->setValue(0);
    this->log_group->setVisible(true);
    this->progress_bar->setVisible(true);

    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto local_time = std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H-%M-%S");

    auto output_base_dir
        = fs::path(output_dir_field->text().toStdString()) / oss.str();

    auto output_dir = output_base_dir;
    auto i = 1;
    while (fs::exists(output_dir)) {
        if (i > 1000) {
            throw std::runtime_error("Failed to create output directory.");
        }
        output_dir = std::string(output_dir) + "_" + std::to_string(i);
        i += 1;
    }

    fs::create_directories(output_dir);
    this->output_path = output_dir;

    QCoreApplication::processEvents();

    QVector<PageTask> tasks;
    for (const QString &file_qstr : input_file_paths) {
        fs::path source_file(file_qstr.toStdString());
        std::string extension = source_file.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(), ::tolower
        );

        try {
            if (extension == ".cbz" || extension == ".cbr") {
                auto temp_archive_dir
                    = fs::path(this->temp_base_dir) / source_file.stem();
                fs::create_directories(temp_archive_dir);

                auto archive = archive_read_new();
                archive_read_support_filter_all(archive);
                archive_read_support_format_all(archive);
                archive_read_open_filename(archive, source_file.c_str(), 10240);

                int page_count = 0;
                struct archive_entry *entry;
                while (archive_read_next_header(archive, &entry)
                       == ARCHIVE_OK) {
                    if (archive_entry_filetype(entry) == AE_IFREG) {
                        page_count += 1;
                    }
                }
                archive_read_close(archive);
                archive_read_free(archive);

                archive_task_counts[file_qstr] = page_count;
                this->total_pages_per_archive[file_qstr] = page_count;
                this->total_pages += page_count;
                this->pages_processed_per_archive[file_qstr] = 0;
                if (page_count == 0) {
                    log_output->setVisible(true);
                    log_output->append(
                        "Archive " + file_qstr + " contains no files."
                    );
                    continue;
                }

                archive = archive_read_new();
                archive_read_support_filter_all(archive);
                archive_read_support_format_all(archive);
                archive_read_open_filename(archive, source_file.c_str(), 10240);

                int i = 0;
                while (archive_read_next_header(archive, &entry)
                       == ARCHIVE_OK) {
                    if (archive_entry_filetype(entry) != AE_IFREG) {
                        continue;
                    }

                    auto task
                        = this->create_task(source_file, temp_archive_dir, i);
                    task.path_in_archive = archive_entry_pathname(entry);

                    fs::path entry_path(task.path_in_archive);
                    task.output_base_name
                        = entry_path.replace_extension("").string();

                    task_queue.enqueue(task);
                    i += 1;
                }
                archive_read_close(archive);
                archive_read_free(archive);
            }
#if defined(PDFIUM_ENABLED)
            else if (extension == ".pdf") {
                FPDF_DOCUMENT doc
                    = FPDF_LoadDocument(source_file.c_str(), nullptr);
                if (!doc) {
                    log_output->setVisible(true);
                    log_output->append(QString(
                                           "Error: Cannot open PDF "
                                           "document %1. Error code: %2"
                    )
                                           .arg(file_qstr)
                                           .arg(FPDF_GetLastError()));
                    continue;
                }

                auto page_count = FPDF_GetPageCount(doc);
                if (page_count == 0) {
                    log_output->setVisible(true);
                    log_output->append(
                        "PDF " + file_qstr + " contains no pages."
                    );
                    FPDF_CloseDocument(doc);
                    continue;
                }

                auto temp_archive_dir
                    = fs::path(this->temp_base_dir) / source_file.stem();
                fs::create_directories(temp_archive_dir);
                archive_task_counts[file_qstr] = page_count;
                this->total_pages_per_archive[file_qstr] = page_count;
                this->total_pages += page_count;
                this->pages_processed_per_archive[file_qstr] = 0;

                for (int i = 0; i < page_count; i += 1) {
                    auto task
                        = this->create_task(source_file, temp_archive_dir, i);
                    task_queue.enqueue(task);
                }
                FPDF_CloseDocument(doc);
            }
#endif
        }
        catch (const std::exception &e) {
            log_output->setVisible(true);
            log_output->append(QString("Error discovering tasks in %1: %2")
                                   .arg(file_qstr, e.what()));
        }
    }

    if (task_queue.isEmpty()) {
        log_output->setVisible(true);
        log_output->append("No pages found to process.");
        this->options.settings_group->setEnabled(true);
        start_button->setEnabled(true);
        cancel_button->setEnabled(false);
        this->progress_bar->setVisible(false);

        // Clean up base temp dir on early exit
        if (!this->temp_base_dir.empty()) {
            try {
                fs::remove_all(this->temp_base_dir);
            }
            catch (const std::exception &e) {
                log_output->setVisible(true);
                log_output->append(
                    QString("Error cleaning up temp directory: %1")
                        .arg(e.what())
                );
            }
            this->temp_base_dir.clear();
        }
        return;
    }

    this->progress_bar->setMaximum(total_pages);

    // Timer
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();
    start_time = ms;
    last_eta_recent_time = ms;
    images_since_last_eta_recent = 0;
    last_progress_value = 0;
    elapsed_label->setText("Elapsed: –");
    eta_overall_label->setText("ETA (overall): –");
    eta_recent_label->setText("ETA (recent): –");
    timer->start(1000);

    for (int i = 0; i < max_concurrent_workers; ++i) {
        start_next_task();
    }
}

void Window::on_cancel_button_clicked() {
    if (is_processing_cancelled)
        return;

    is_processing_cancelled = true;

    task_queue.clear();

    for (QProcess *p : running_processes) {
        p->kill();
    }
    running_processes.clear();
    running_tasks.clear();

    for (QWidget *widget : this->active_file_widgets.values()) {
        this->progress_bars_layout->removeWidget(widget);
        delete widget;
    }
    this->active_file_widgets.clear();
    this->active_progress_bars.clear();
    this->file_elapsed_labels.clear();
    this->file_eta_overall_labels.clear();
    this->file_eta_recent_labels.clear();
    this->file_timers.clear();
    this->pages_processed_per_archive.clear();
    this->progress_bars_group->setVisible(false);

    this->progress_bar->setVisible(false);
    this->progress_bar->setValue(0);

    timer->stop();
    this->options.settings_group->setEnabled(true);
    start_button->setEnabled(true);
    cancel_button->setEnabled(false);

    // Clean up base temp dir on cancel
    if (!this->temp_base_dir.empty()) {
        try {
            fs::remove_all(this->temp_base_dir);
        }
        catch (const std::exception &e) {
            log_output->setVisible(true);
            log_output->append(
                QString("Error cleaning up temp directory: %1").arg(e.what())
            );
        }
        this->temp_base_dir.clear();
    }

    if (fs::exists(this->output_path) && fs::is_directory(this->output_path)
        && fs::is_empty(this->output_path)) {
        fs::remove_all(this->output_path);
    }
}

void Window::on_worker_finished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    running_processes.removeAll(process);
    if (!running_tasks.contains(process)) {
        process->deleteLater();
        return;
    }
    PageTask finished_task = running_tasks.take(process);

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        log_output->setVisible(true);
        log_output->append(
            QString("Worker process failed or crashed. Exit code: %1")
                .arg(exitCode)
        );
    }

    handle_task_finished();

    QString source_qstr
        = QString::fromStdString(finished_task.source_file.string());

    if (this->pages_processed_per_archive.contains(source_qstr)) {
        this->pages_processed_per_archive[source_qstr]++;
        if (this->file_timers.contains(source_qstr)) {
            this->file_timers[source_qstr].images_since_last_eta_recent++;
        }
        if (this->active_progress_bars.contains(source_qstr)) {
            auto progressBar = this->active_progress_bars.value(source_qstr);
            progressBar->setValue(
                this->pages_processed_per_archive.value(source_qstr)
            );
        }
    }

    if (archive_task_counts.contains(source_qstr)) {
        archive_task_counts[source_qstr] -= 1;
        if (archive_task_counts[source_qstr] == 0) {
            archive_task_counts.remove(source_qstr);
            create_archive(source_qstr);

            if (this->active_file_widgets.contains(source_qstr)) {
                auto widget = this->active_file_widgets.take(source_qstr);
                this->progress_bars_layout->removeWidget(widget);
                delete widget;
                this->active_progress_bars.remove(source_qstr);
                this->file_elapsed_labels.remove(source_qstr);
                this->file_eta_overall_labels.remove(source_qstr);
                this->file_eta_recent_labels.remove(source_qstr);
                this->file_timers.remove(source_qstr);
                this->total_pages_per_archive.remove(source_qstr);

                if (this->active_file_widgets.isEmpty()) {
                    this->progress_bars_group->setVisible(false);
                }
            }
        }
    }

    if (is_processing_cancelled) {
        if (running_processes.isEmpty()) {
            log_output->setVisible(true);
            log_output->append("All running tasks have been cancelled.");
        }
    }
    else {
        if (pages_processed == total_pages) {
            timer->stop();
            this->options.settings_group->setEnabled(true);
            start_button->setEnabled(true);
            cancel_button->setEnabled(false);

            // Clean up base temp dir on success
            if (!this->temp_base_dir.empty()) {
                try {
                    fs::remove_all(this->temp_base_dir);
                }
                catch (const std::exception &e) {
                    log_output->setVisible(true);
                    log_output->append(
                        QString("Error cleaning up temp directory: %1")
                            .arg(e.what())
                    );
                }
                this->temp_base_dir.clear();
            }
        }
        else {
            start_next_task();
        }
    }

    process->deleteLater();
}

void Window::on_worker_output() {
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (process) {
        // Read line by line to prevent partial messages
        while (process->canReadLine()) {
            log_output->setVisible(true);
            log_output->append(process->readLine().trimmed());
        }
    }
}
