#include "include/window.hpp"
#include "include/display_presets.hpp"
#include "include/task.hpp"
#include "include/ui_constants.hpp"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <archive.h>
#include <archive_entry.h>
#include <chrono>
#include <fpdfview.h>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <thread>

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

    this->setWindowTitle("Comicpress");
    central_widget = new QWidget(this);
    this->setCentralWidget(central_widget);
    main_layout = new QVBoxLayout(central_widget);

    this->setup_ui();
    this->on_display_preset_changed();
    this->on_enable_image_scaling_changed(
        this->enable_image_scaling_check_box->checkState()
    );
    this->connect_signals();
}

void Window::update_time_labels() {
    if (!this->start_time.has_value()) {
        return;
    }

    auto start_time = this->start_time.value();

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();

    auto elapsed = ms - start_time;
    auto elapsed_str = "Elapsed: " + time_to_str(elapsed);
    this->elapsed_label->setText(QString::fromStdString(elapsed_str));

    auto value = this->progress_bar->value();

    // Overall ETA
    if (value > 0) {
        auto total = this->progress_bar->maximum();
        auto per_unit = static_cast<double>(elapsed) / value;
        auto remaining = static_cast<double>(total - value) * per_unit;
        auto eta_overall_str
            = "ETA (overall): " + time_to_str(static_cast<int64_t>(remaining));
        this->eta_overall_label->setText(
            QString::fromStdString(eta_overall_str)
        );
    }

    // Recent ETA
    if (this->last_eta_recent_time.has_value()
        && ms - this->last_eta_recent_time.value() >= 1000
        && this->images_since_last_eta_recent > 0) {
        auto interval = ms - this->last_eta_recent_time.value();
        auto speed = static_cast<double>(this->images_since_last_eta_recent)
                   / static_cast<double>(interval);
        auto total_remaining = this->progress_bar->maximum() - value;
        auto remaining = static_cast<double>(total_remaining) / speed;

        this->eta_recent_intervals.push_front(static_cast<int64_t>(remaining));

        this->last_eta_recent_time = ms;
        this->images_since_last_eta_recent = 0;
    }

    if (!this->eta_recent_intervals.dq.empty()) {
        auto dq = this->eta_recent_intervals.dq;
        auto sum = std::accumulate(dq.begin(), dq.end(), 0LL);
        auto avg_eta = static_cast<double>(sum) / dq.size();
        auto eta_recent_str
            = "ETA (recent): " + time_to_str(static_cast<int64_t>(avg_eta));
        this->eta_recent_label->setText(QString::fromStdString(eta_recent_str));
    }
}

void Window::setup_ui() {
    auto io_group = this->create_io_group();
    auto settings_group = this->create_settings_group();
    auto log_group = this->create_log_group();

    this->main_layout->addWidget(io_group);
    this->main_layout->addWidget(settings_group);
    this->main_layout->addWidget(log_group);
}

QGroupBox *Window::create_io_group() {
    auto io_group = new QGroupBox();
    auto io_layout = new QVBoxLayout(io_group);
    auto file_buttons_layout = new QHBoxLayout();

    this->file_list = new QListWidget();

    this->add_files_button = new QPushButton("Add files");
    this->remove_selected_button = new QPushButton("Remove selected");
    this->clear_all_button = new QPushButton("Clear all");

    this->remove_selected_button->setEnabled(false);
    this->clear_all_button->setEnabled(false);

    file_buttons_layout->addWidget(this->add_files_button);
    file_buttons_layout->addWidget(this->remove_selected_button);
    file_buttons_layout->addWidget(this->clear_all_button);

    io_layout->addWidget(new QLabel("Input files"));
    io_layout->addWidget(file_list);
    io_layout->addLayout(file_buttons_layout);

    auto output_layout = new QHBoxLayout();
    this->output_dir_field = new QLineEdit(".");
    this->browse_output_button = new QPushButton("Browse");
    output_layout->addWidget(new QLabel("Output directory"));
    output_layout->addWidget(this->output_dir_field);
    output_layout->addWidget(this->browse_output_button);

    io_layout->addLayout(output_layout);

    return io_group;
}

QGroupBox *Window::create_settings_group() {
    auto settings_group = new QGroupBox("Processing parameters");
    this->settings_layout = new QFormLayout(settings_group);

    this->add_pdf_pixel_density_widget();
    this->add_contrast_widget();
    this->add_display_presets_widget();
    this->add_scaling_widgets();
    this->add_parallel_workers_widget();

    return settings_group;
}

QGroupBox *Window::create_log_group() {
    auto log_group = new QGroupBox();
    auto log_layout = new QVBoxLayout(log_group);

    this->progress_bar = new QProgressBar();
    this->progress_bar->setValue(0);
    this->progress_bar->setTextVisible(true);
    this->progress_bar->setFormat("%p %");

    auto time_layout = new QHBoxLayout();
    this->elapsed_label = new QLabel("Elapsed: –");
    this->eta_overall_label = new QLabel("ETA (overall): –");
    this->eta_recent_label = new QLabel("ETA (recent): –");
    time_layout->addWidget(this->elapsed_label);
    time_layout->addWidget(this->eta_overall_label);
    time_layout->addWidget(this->eta_recent_label);

    this->log_output = new QTextEdit();
    this->log_output->setReadOnly(true);
    this->log_output->setFontFamily("monospace");

    auto action_layout = new QHBoxLayout();
    this->start_button = new QPushButton("Start");
    this->cancel_button = new QPushButton("Cancel");
    this->cancel_button->setEnabled(false);
    action_layout->addWidget(this->start_button);
    action_layout->addWidget(this->cancel_button);

    log_layout->addWidget(this->progress_bar);
    log_layout->addLayout(time_layout);
    log_layout->addWidget(this->log_output);
    log_layout->addLayout(action_layout);

    return log_group;
}

void Window::add_pdf_pixel_density_widget() {
    this->pdf_pixel_density_spin_box = new QSpinBox();
    this->pdf_pixel_density_spin_box->setRange(300, 4'800);
    this->pdf_pixel_density_spin_box->setValue(1'200);
    this->pdf_pixel_density_spin_box->setSingleStep(300);

    auto pdf_pixel_density_widget = this->create_widget_with_info(
        new QLabel("PDF pixel density (PPI)"), PDF_TOOLTIP
    );

    this->settings_layout->addRow(
        pdf_pixel_density_widget, this->pdf_pixel_density_spin_box
    );
}

void Window::add_contrast_widget() {
    auto contrast_widget = new QWidget();
    auto contrast_layout = new QHBoxLayout(contrast_widget);
    contrast_layout->setContentsMargins(0, 0, 0, 0);
    contrast_layout->setSpacing(20);

    this->contrast_check_box = new QCheckBox("Stretch contrast");
    this->contrast_check_box->setChecked(true);
    contrast_widget = this->create_widget_with_info(
        this->contrast_check_box, "Lorem ipsum"
    );

    contrast_layout->addStretch();
    this->settings_layout->addRow(contrast_widget);
}

void Window::add_display_presets_widget() {
    auto display_preset = QString::fromStdString(this->display_preset.brand);
    this->display_preset_button = new QPushButton(display_preset);
    auto display_menu = new QMenu(this);

    if (auto custom_action = display_menu->addAction("Custom")) {
        connect(custom_action, &QAction::triggered, this, [this]() {
            this->set_display_preset("Custom", "");
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

    this->display_preset_button->setMenu(display_menu);
    this->settings_layout->addRow(
        "Display preset", this->display_preset_button
    );
}

void Window::add_scaling_widgets() {
    auto scaling_widget = new QWidget();
    auto scaling_layout = new QHBoxLayout(scaling_widget);
    scaling_layout->setContentsMargins(0, 0, 0, 0);
    scaling_layout->setSpacing(20);

    this->enable_image_scaling_check_box = new QCheckBox("Scale images");
    auto enable_image_scaling_label_widget = this->create_widget_with_info(
        this->enable_image_scaling_check_box, "Lorem ipsum"
    );

    // Width
    this->width_spin_box = new QSpinBox();
    this->width_spin_box->setRange(100, 4'000);
    auto width_container = new QWidget();
    auto width_layout = new QHBoxLayout(width_container);
    width_layout->setContentsMargins(0, 0, 0, 0);
    width_layout->setSpacing(4);
    width_layout->addWidget(new QLabel("Width"));
    width_layout->addWidget(this->width_spin_box);

    // Height
    this->height_spin_box = new QSpinBox();
    this->height_spin_box->setRange(100, 4'000);
    auto height_container = new QWidget();
    auto height_layout = new QHBoxLayout(height_container);
    height_layout->setContentsMargins(0, 0, 0, 0);
    height_layout->setSpacing(4);
    height_layout->addWidget(new QLabel("Height"));
    height_layout->addWidget(this->height_spin_box);

    // Resampler
    auto resampler_widget = new QWidget();
    auto resampler_layout = new QHBoxLayout(resampler_widget);
    resampler_layout->setContentsMargins(0, 0, 0, 0);
    resampler_layout->setSpacing(4);
    this->resampler_combo_box = new QComboBox();
    this->resampler_combo_box->addItems(
        {"Lanczos 3", "Magic Kernel Sharp 2021"}
    );
    this->resampler_combo_box->setCurrentText("Magic Kernel Sharp 2021");

    // Add resampler widgets.
    auto resampler_label_widget = this->create_widget_with_info(
        new QLabel("Resampling filter"), "Lorem"
    );
    resampler_layout->addWidget(resampler_label_widget);
    resampler_layout->addWidget(this->resampler_combo_box);

    scaling_layout->addWidget(enable_image_scaling_label_widget);
    scaling_layout->addWidget(width_container);
    scaling_layout->addWidget(height_container);
    scaling_layout->addWidget(resampler_widget);
    scaling_layout->addStretch();

    this->settings_layout->addRow(scaling_widget);
}

void Window::add_parallel_workers_widget() {
    this->workers_spin_box = new QSpinBox();
    auto threads = std::thread::hardware_concurrency();
    this->workers_spin_box->setRange(1, threads);
    this->workers_spin_box->setValue(threads);
    this->settings_layout->addRow("Parallel workers", this->workers_spin_box);
}

void Window::on_add_files_clicked() {
    auto files = QFileDialog::getOpenFileNames(
        this, "Select input files", "", "Supported files (*.pdf *.cbz *.cbr)"
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
}

void Window::on_display_preset_changed() {
    auto brand = this->display_preset.brand;
    auto model = this->display_preset.model;
    auto is_custom = model.length() == 0;

    this->enable_image_scaling_check_box->setEnabled(is_custom);
    this->enable_image_scaling_check_box->setChecked(!is_custom);

    if (!is_custom) {
        auto display = DISPLAY_PRESETS.at(brand).value().at(model);
        this->width_spin_box->setValue(display.width);
        this->height_spin_box->setValue(display.height);
    }
}

void Window::on_enable_image_scaling_changed(int state) {
    auto enabled = state == Qt::Checked;
    this->width_spin_box->setEnabled(enabled);
    this->height_spin_box->setEnabled(enabled);
    this->resampler_combo_box->setEnabled(enabled);
}

void Window::on_start_button_clicked() {
    QStringList input_file_paths;
    for (int i = 0; i < file_list->count(); i += 1) {
        input_file_paths.append(
            file_list->item(i)->data(Qt::UserRole).toString()
        );
    }

    if (input_file_paths.isEmpty()) {
        log_output->append("No input files selected.");
        return;
    }

    start_button->setEnabled(false);
    cancel_button->setEnabled(true);
    log_output->clear();
    task_queue.clear();
    running_processes.clear();
    is_processing_cancelled = false;
    files_processed = 0;
    max_concurrent_workers = workers_spin_box->value();

    fs::path output_dir = fs::path(output_dir_field->text().toStdString());
    fs::create_directories(output_dir);
    log_output->append(
        "Output directory: " + QString::fromStdString(output_dir.string())
    );

    log_output->append("Discovering pages in all files...");
    QCoreApplication::processEvents();

    QVector<PageTask> tasks;
    for (const QString &file_qstr : input_file_paths) {
        fs::path source_file(file_qstr.toStdString());
        std::string extension = source_file.extension().string();
        std::transform(
            extension.begin(), extension.end(), extension.begin(), ::tolower
        );

        try {
            if (extension == ".pdf") {
                FPDF_DOCUMENT doc
                    = FPDF_LoadDocument(source_file.c_str(), nullptr);
                if (!doc) {
                    log_output->append(
                        QString(
                            "Error: Cannot open PDF document %1. Error code: %2"
                        )
                            .arg(file_qstr)
                            .arg(FPDF_GetLastError())
                    );
                    continue;
                }

                auto page_count = FPDF_GetPageCount(doc);
                for (int i = 0; i < page_count; i += 1) {
                    PageTask task;
                    task.source_file = source_file;
                    task.output_dir = output_dir;
                    task.page_number = i;
                    // TODO: Fix this abomination.
                    task.output_base_name
                        = QString("%1_page_%2")
                              .arg(
                                  QString::fromStdString(
                                      source_file.stem().string()
                                  )
                              )
                              .arg(i + 1, 4, 10, QChar('0'))
                              .toStdString();
                    task_queue.enqueue(task);
                }
                FPDF_CloseDocument(doc);
            }
            else if (extension == ".cbz" || extension == ".cbr") {
                auto archive = archive_read_new();
                archive_read_support_filter_all(archive);
                archive_read_support_format_all(archive);
                archive_read_open_filename(archive, source_file.c_str(), 10240);

                struct archive_entry *entry;
                int i = 0;
                // while (archive_read_next_header(archive, &entry) ==
                // ARCHIVE_OK) {
                while (true) {
                    auto archive_read
                        = archive_read_next_header(archive, &entry);
                    if (archive_read != ARCHIVE_OK) {
                        break;
                    }
                    if (archive_entry_filetype(entry) != AE_IFREG) {
                        continue;
                    }

                    PageTask task;
                    task.source_file = source_file;
                    task.output_dir = output_dir;
                    task.path_in_archive = archive_entry_pathname(entry);
                    // TODO: Fix this abomination.
                    task.output_base_name
                        = QString("%1_page_%2")
                              .arg(
                                  QString::fromStdString(
                                      source_file.stem().string()
                                  )
                              )
                              .arg(i + 1, 4, 10, QChar('0'))
                              .toStdString();
                    task_queue.enqueue(task);
                    i += 1;
                }
                archive_read_close(archive);
                archive_read_free(archive);
            }
        }
        catch (const std::exception &e) {
            log_output->append(QString("Error discovering tasks in %1: %2")
                                   .arg(file_qstr, e.what()));
        }
    }

    if (task_queue.isEmpty()) {
        log_output->append("No pages found to process.");
        start_button->setEnabled(true);
        cancel_button->setEnabled(false);
        return;
    }

    total_files_to_process = task_queue.count();
    this->progress_bar->setValue(0);
    this->progress_bar->setMaximum(total_files_to_process);
    this->progress_bar->setFormat("%p % (%v / %m pages)");
    log_output->append(QString("Found %1 pages. Starting processing...")
                           .arg(total_files_to_process));

    // Timer
    auto now = std::chrono::system_clock::now();
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
    log_output->append("\n🛑 Cancelling processing...");

    task_queue.clear();

    for (QProcess *p : running_processes) {
        p->kill();
    }
    running_processes.clear();

    timer->stop();
    start_button->setEnabled(true);
    cancel_button->setEnabled(false);
    log_output->append("Processing cancelled.");
}

void Window::start_next_task() {
    if (task_queue.isEmpty()
        || running_processes.size() >= max_concurrent_workers
        || is_processing_cancelled) {
        return;
    }

    PageTask task = task_queue.dequeue();

    QProcess *process = new QProcess(this);
    running_processes.append(process);

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

    QString program
        = QCoreApplication::applicationDirPath() + "/comicpress-worker";
    QStringList arguments;
    arguments << QString::fromStdString(task.source_file.string())
              << QString::fromStdString(task.output_dir.string())
              << QString::fromStdString(task.output_base_name)
              << QString::number(task.page_number)
              << (task.path_in_archive.empty()
                      ? "NULL"
                      : QString::fromStdString(task.path_in_archive));

    process->start(program, arguments);
}

void Window::on_worker_finished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    running_processes.removeAll(process);

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        log_output->append(
            QString("Worker process failed or crashed. Exit code: %1")
                .arg(exitCode)
        );
    }

    handle_task_finished();

    if (is_processing_cancelled) {
        if (running_processes.isEmpty()) {
            log_output->append("All running tasks have been cancelled.");
        }
    }
    else {
        if (files_processed == total_files_to_process) {
            log_output->append("\n✅ All processing complete.");
            timer->stop();
            start_button->setEnabled(true);
            cancel_button->setEnabled(false);
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
            log_output->append(process->readLine().trimmed());
        }
    }
}

void Window::handle_log_message(const QString &message) {
    log_output->append(message);
}

void Window::handle_task_finished() {
    files_processed++;
    images_since_last_eta_recent++;
    progress_bar->setValue(files_processed);
}

QWidget *Window::create_widget_with_info(
    QWidget *main_widget, const char *tooltip_text
) {
    auto container = new QWidget();
    auto layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto info_icon_label = new QLabel();
    if (auto style = this->style()) {
        auto icon = style->standardIcon(
            QStyle::StandardPixmap::SP_MessageBoxInformation
        );
        std::string formatted_tooltip
            = "<p>" + std::string(tooltip_text) + "</p>";
        info_icon_label->setPixmap(icon.pixmap(16, 16));
        info_icon_label->setToolTip(QString::fromStdString(formatted_tooltip));
        layout->addWidget(info_icon_label);
    }
    layout->addWidget(main_widget);
    return container;
}

void Window::connect_signals() {
    connect(
        this->add_files_button,
        &QPushButton::clicked,
        this,
        &Window::on_add_files_clicked
    );
    connect(
        this->enable_image_scaling_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_enable_image_scaling_changed
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
}

void Window::set_display_preset(std::string brand, std::string model) {
    this->display_preset.brand = brand;
    this->display_preset.model = model;
    QString text = model.empty() ? QString::fromStdString(brand)
                                 : QString::fromStdString(brand + " " + model);
    this->display_preset_button->setText(text);
    this->on_display_preset_changed();
}

Window::~Window() {
}

std::string time_to_str(int64_t millis) {
    int seconds = static_cast<int>(millis / 1000);

    int h = seconds / 3600;
    int rem = seconds % 3600;
    int m = rem / 60;
    int s = rem % 60;

    std::ostringstream oss;
    if (seconds < 60) {
        oss << s << " s";
    }
    else if (seconds < 3600) {
        oss << m << " min " << std::setw(2) << std::setfill('0') << s << " s";
    }
    else {
        oss << h << " h " << std::setw(2) << std::setfill('0') << m << " min "
            << std::setw(2) << std::setfill('0') << s << " s";
    }

    return oss.str();
}
