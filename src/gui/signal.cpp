#include "include/display_presets.hpp"
#include "include/window.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <archive.h>
#include <archive_entry.h>

#if defined(PDF_ENABLED)
#include <fpdfview.h>
#endif

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef __linux__
#include <sys/xattr.h>
#include <vector>
#endif

// Turns a Flatpak file-portal path (`/run/user/...`) into the real path the
// user recognizes (`/home/user/...`), which the portal stashes in an extended
// attribute. Returns `path` untouched when that attribute is absent: outside
// Flatpak, and on non-Linux platforms that have no portal.
static QString resolve_host_path(const QString &path) {
#ifdef __linux__
    auto attr = "user.document-portal.host-path";
    auto local = QFile::encodeName(path);

    // A null buffer makes getxattr return the value's length; use it to size an
    // exact buffer (the stored value has no null terminator).
    auto size = getxattr(local.constData(), attr, nullptr, 0);
    if (size <= 0) {
        return path;
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    auto read = getxattr(local.constData(), attr, buffer.data(), buffer.size());
    if (read <= 0) {
        return path;
    }

    return QFile::decodeName(QByteArray(buffer.data(), static_cast<int>(read)));
#else
    return path;
#endif
}

static bool is_dir_writable(const QString &path) {
    if (path.isEmpty()) {
        return false;
    }
    QFileInfo info(path);
    return info.isDir() && info.isWritable();
}

static QString choose_directory(
    QWidget *parent, const QString &caption, const QString &start_dir
) {
    QFileDialog dialog(parent, caption);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontResolveSymlinks, true);

    if (!start_dir.isEmpty()) {
        dialog.setDirectoryUrl(QUrl::fromLocalFile(start_dir));
    }

    if (dialog.exec() != QDialog::Accepted) {
        return QString();
    }

    auto selected = dialog.selectedUrls();
    return selected.isEmpty() ? QString() : selected.constFirst().toLocalFile();
}

static bool create_output_dir(fs::path &output_dir) {
    auto output_parent = output_dir.parent_path();
    auto err_code = std::error_code{};
    if (!output_parent.empty()) {
        fs::create_directories(output_parent, err_code);
        if (err_code) {
            return false;
        }
    }

    auto output_base_dir = output_dir;
    auto i = 1;
    while (!fs::create_directory(output_dir, err_code)) {
        if (i > 1000 || (err_code && err_code != std::errc::file_exists)) {
            return false;
        }
        output_dir = output_base_dir;
        output_dir += "_" + std::to_string(i);
        i += 1;
    }

    return true;
}

void Window::connect_signals() {
    connect(
        add_files_button_,
        &QPushButton::clicked,
        this,
        &Window::on_add_files_clicked
    );
    connect(
        remove_selected_button_,
        &QPushButton::clicked,
        this,
        &Window::on_remove_selected_clicked
    );
    connect(
        clear_all_button_,
        &QPushButton::clicked,
        this,
        &Window::on_clear_all_clicked
    );
    connect(
        browse_output_button_,
        &QPushButton::clicked,
        this,
        &Window::on_browse_output_clicked
    );
    connect(
        options_.output_format_combo_box,
        &QComboBox::currentIndexChanged,
        this,
        &Window::on_output_format_combo_box_changed
    );
#if defined(PDF_ENABLED)
    connect(
        options_.pdf_pixel_density_combo_box,
        &QComboBox::currentIndexChanged,
        this,
        &Window::on_pdf_pixel_density_combo_box_changed
    );
#endif
    connect(
        options_.double_page_spread_combo_box,
        &QComboBox::currentIndexChanged,
        this,
        &Window::on_double_page_spread_changed
    );
    connect(
        options_.convert_to_greyscale,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_preset_option_modified
    );
    connect(
        options_.bit_depth_combo_box,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        options_.width_spin_box,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        options_.height_spin_box,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        &Window::on_preset_option_modified
    );
    connect(
        options_.enable_image_scaling_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_preset_option_modified
    );
    connect(
        options_.advanced_options_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_advanced_options_changed
    );
    connect(
        options_.enable_image_scaling_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_enable_image_scaling_changed
    );
    connect(
        options_.enable_image_quantization_check_box,
        &QCheckBox::checkStateChanged,
        this,
        &Window::on_enable_image_quantization_changed
    );
    connect(
        options_.image_format_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_image_format_changed
    );
    connect(
        options_.image_compression_spin_box,
        &QSpinBox::valueChanged,
        this,
        &Window::on_image_compression_changed
    );
    connect(
        options_.image_compression_type_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_image_compression_type_changed_explicit
    );
    connect(
        options_.image_quality_spin_box,
        &QDoubleSpinBox::valueChanged,
        this,
        &Window::on_image_quality_changed
    );
    connect(
        options_.image_quality_label_jpeg_xl,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_jpeg_xl_quality_type_changed
    );
    connect(
        start_button_,
        &QPushButton::clicked,
        this,
        &Window::on_start_button_clicked
    );
    connect(
        cancel_button_,
        &QPushButton::clicked,
        this,
        &Window::on_cancel_button_clicked
    );
    connect(
        file_list_,
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
#if defined(PDF_ENABLED)
        "Supported files (*.pdf *.cbz *.cbr)"
#else
        "Supported files (*.cbz *.cbr)"
#endif
    );

    if (files.isEmpty()) {
        return;
    }

    QStringList existing_paths;
    for (int i = 0; i < file_list_->count(); i += 1) {
        if (auto item = file_list_->item(i)) {
            if (auto data = item->data(Qt::UserRole); data.isValid()) {
                existing_paths.append(data.toString());
            }
        }
    }

    for (const QString &file : files) {
        if (!existing_paths.contains(file)) {
            auto item = new QListWidgetItem(QFileInfo(file).fileName());
            item->setData(Qt::UserRole, file);
            file_list_->addItem(item);
        }
    }

    update_file_list_buttons();
}

void Window::on_remove_selected_clicked() {
    qDeleteAll(file_list_->selectedItems());
    update_file_list_buttons();
}

void Window::on_clear_all_clicked() {
    file_list_->clear();
    update_file_list_buttons();
}

void Window::on_browse_output_clicked() {
    // Start where the user already is, or in Documents the first time.
    QString start_dir = output_dir_field_->text().isEmpty()
                          ? QStandardPaths::writableLocation(
                                QStandardPaths::DocumentsLocation
                            )
                          : output_dir_field_->text();

    QString dir = choose_directory(this, tr("Select output folder"), start_dir);

    if (!dir.isEmpty()) {
        set_output_dir(dir);
    }
}

void Window::set_output_dir(const QString &io_path) {
    output_dir_io_path_ = io_path;
    output_dir_field_->setText(resolve_host_path(io_path));
    persist_output_dir();
}

void Window::persist_output_dir() {
    QSettings settings;
    settings.setValue("output/io_path", output_dir_io_path_);
    settings.setValue("output/host_path", output_dir_field_->text());
}

void Window::restore_output_dir() {
    QSettings settings;
    auto io_path = settings.value("output/io_path").toString();
    auto host_path = settings.value("output/host_path").toString();

    if (is_dir_writable(io_path)) {
        output_dir_io_path_ = io_path;
        output_dir_field_->setText(
            host_path.isEmpty() ? resolve_host_path(io_path) : host_path
        );
        return;
    }

    if (is_dir_writable(host_path)) {
        output_dir_io_path_ = host_path;
        output_dir_field_->setText(host_path);
        return;
    }

    output_dir_io_path_.clear();
    output_dir_field_->clear();
}

bool Window::ensure_output_dir() {
    if (is_dir_writable(output_dir_io_path_)) {
        return true;
    }

    QString dir = choose_directory(
        this,
        tr("Choose an output folder"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    );

    if (dir.isEmpty()) {
        return false;
    }

    set_output_dir(dir);
    return true;
}

QString Window::effective_output_dir() const {
    return output_dir_io_path_;
}

void Window::on_output_format_combo_box_changed() {
    auto image_format_combo = options_.image_format_combo_box;
    auto *view = qobject_cast<QListView *>(image_format_combo->view());
    if (!view) {
        return;
    }

    static constexpr ImageFormat EPUB_UNSUPPORTED[]
        = {ImageFormat::AVIF, ImageFormat::JPEG_XL};

    auto output_format
        = options_.output_format_combo_box->currentData().value<OutputFormat>();
    auto is_epub = output_format == OutputFormat::EPUB;
    auto image_format = current_image_format();
    if (is_epub && std::ranges::contains(EPUB_UNSUPPORTED, image_format)) {
        image_format_combo->setCurrentIndex(
            image_format_combo->findData(QVariant::fromValue(ImageFormat::PNG))
        );
    }
    for (auto format : EPUB_UNSUPPORTED) {
        view->setRowHidden(
            image_format_combo->findData(QVariant::fromValue(format)), is_epub
        );
    }
}

ImageFormat Window::current_image_format() const {
    return options_.image_format_combo_box->currentData().value<ImageFormat>();
}

#if defined(PDF_ENABLED)
void Window::on_pdf_pixel_density_combo_box_changed() {
    auto spin = options_.pdf_pixel_density_spin_box;
    auto quality = options_.pdf_pixel_density_combo_box->currentData()
                       .value<PdfQuality>();
    auto is_custom = quality == PdfQuality::CUSTOM;
    spin->setVisible(is_custom);
    if (!is_custom) {
        spin->setValue(static_cast<int>(quality));
    }
}
#endif

void Window::on_double_page_spread_changed() {
    auto action = options_.double_page_spread_combo_box->currentData()
                      .value<DoublePageSpreadActions>();
    auto should_show = action == DoublePageSpreadActions::ROTATE
                    || action == DoublePageSpreadActions::BOTH;
    options_.rotation_options_container->setVisible(should_show);
}

void Window::on_preset_option_modified() {
    if (is_programmatically_changing_values_) {
        return;
    }

    if (display_preset.brand != "None") {
        set_display_preset("None", "");
    }
}

void Window::on_display_preset_changed() {
    auto brand = display_preset.brand;
    auto model = display_preset.model;
    auto is_custom = brand == "None";

    if (is_custom) {
        return;
    }

    is_programmatically_changing_values_ = true;

    const auto &models = DISPLAY_PRESETS.at(brand).value();
    auto it = std::find_if(
        models.begin(), models.end(), [&model](const auto &pair) {
            return pair.first == model;
        }
    );

    if (it != models.end()) {
        const DisplaySpec &display = it->second;
        options_.enable_image_scaling_check_box->setChecked(true);
        options_.width_spin_box->setValue(display.width);
        options_.height_spin_box->setValue(display.height);
        options_.bit_depth_combo_box->setCurrentIndex(display.bit_depth_index);
        options_.convert_to_greyscale->setChecked(!display.colour);
    }

    is_programmatically_changing_values_ = false;
}

void Window::on_advanced_options_changed(int state) {
    bool is_checked = state == Qt::Checked;

    options_.linear_light_resampling_label->setVisible(is_checked);
    options_.linear_light_resampling_container->setVisible(is_checked);

    options_.quantize_pages_label->setVisible(is_checked);
    options_.quantize_pages_container->setVisible(is_checked);

    options_.scale_pages_label->setVisible(is_checked);
    options_.scale_pages_container->setVisible(is_checked);

    options_.image_format_label->setVisible(is_checked);
    options_.image_format_container->setVisible(is_checked);

    if (is_checked) {
        on_enable_image_quantization_changed(
            options_.enable_image_quantization_check_box->checkState()
        );
        on_enable_image_scaling_changed(
            options_.enable_image_scaling_check_box->checkState()
        );
        on_image_format_changed();
    }
    else {
        options_.quantization_options_container->setVisible(false);
        options_.scaling_options_container->setVisible(false);
        options_.image_format_options_container->setVisible(false);
    }

    options_.workers_label->setVisible(is_checked);
    options_.workers_spin_box->setVisible(is_checked);
}

void Window::on_enable_image_scaling_changed(int state) {
    bool is_checked = state == Qt::Checked;
    bool parent_visible = options_.scale_pages_container->isVisible();
    options_.scaling_options_container->setVisible(
        is_checked && parent_visible
    );
}

void Window::on_enable_image_quantization_changed(int state) {
    bool is_checked = state == Qt::Checked;
    options_.quantization_options_container->setVisible(is_checked);
    auto combo = options_.image_compression_type_combo_box;
    if (is_checked) {
        combo->setCurrentIndex(
            combo->findData(QVariant::fromValue(CompressionType::LOSSLESS))
        );
    }
    else if (!compression_type_changed_) {
        combo->setCurrentIndex(
            combo->findData(QVariant::fromValue(CompressionType::LOSSY))
        );
    }
}

void Window::on_image_format_changed() {
    options_.image_format_options_container->setVisible(true);

    auto img_format = current_image_format();
    const auto &settings = format_settings_.at(img_format);
    auto is_jpeg_xl = img_format == ImageFormat::JPEG_XL;

    // Important: This should be before the others.
    on_jpeg_xl_quality_type_changed();

    options_.image_compression_spin_box->setRange(
        settings.compression_effort_min, settings.compression_effort_max
    );
    options_.image_compression_spin_box->setValue(settings.compression_effort);
    options_.image_quality_label
        = is_jpeg_xl
            ? static_cast<QWidget *>(options_.image_quality_label_jpeg_xl)
            : options_.image_quality_label_original;
    options_.image_quality_label_original->setVisible(!is_jpeg_xl);
    options_.image_quality_label_jpeg_xl->setVisible(is_jpeg_xl);

    options_.image_quality_spin_box->setValue(
        jpeg_xl_distance_selected() ? jpeg_xl_distance_ : settings.quality
    );

    options_.image_compression_type_label->setVisible(
        settings.has_compression_type
    );
    options_.image_compression_type_combo_box->setVisible(
        settings.has_compression_type
    );
    options_.image_compression_type_tooltip->setVisible(
        settings.has_compression_type
    );
    options_.image_compression_label->setVisible(
        settings.has_compression_effort
    );
    options_.image_compression_spin_box->setVisible(
        settings.has_compression_effort
    );

    on_image_compression_type_changed(false);
}

void Window::on_image_compression_changed(int state) {
    format_settings_[current_image_format()].compression_effort = state;
}

void Window::on_image_compression_type_changed_explicit() {
    on_image_compression_type_changed(true);
}

void Window::on_image_compression_type_changed(bool is_explicit) {
    auto img_format = current_image_format();
    auto compression_type
        = options_.image_compression_type_combo_box->currentData()
              .value<CompressionType>();
    auto is_lossy = compression_type == CompressionType::LOSSY;

    auto image_quality_visible = img_format != ImageFormat::PNG
                              && (img_format == ImageFormat::JPEG || is_lossy);
    auto jpeg_xl_quality_tooltip_visible
        = img_format == ImageFormat::JPEG_XL && is_lossy;

    options_.image_quality_label->setVisible(image_quality_visible);
    options_.image_quality_spin_box->setVisible(image_quality_visible);
    options_.image_quality_jpeg_xl_tooltip->setVisible(
        jpeg_xl_quality_tooltip_visible
    );

    if (is_explicit) {
        compression_type_changed_ = true;
    }
}

void Window::on_image_quality_changed(double value) {
    if (current_image_format() == ImageFormat::JPEG_XL
        && jpeg_xl_distance_selected()) {
        jpeg_xl_distance_ = value;
        return;
    }
    format_settings_[current_image_format()].quality = static_cast<int>(value);
}

bool Window::jpeg_xl_distance_selected() const {
    return options_.image_quality_label_jpeg_xl->currentData()
               .value<QualityType>()
        == QualityType::DISTANCE;
}

void Window::on_jpeg_xl_quality_type_changed() {
    auto *spin_box = options_.image_quality_spin_box;
    if (jpeg_xl_distance_selected()) {
        spin_box->setRange(0.0, 15.0);
        spin_box->setSingleStep(0.1);
        spin_box->setDecimals(2);
        spin_box->setValue(jpeg_xl_distance_);
    }
    else {
        spin_box->setRange(0.0, 100.0);
        spin_box->setSingleStep(1.0);
        spin_box->setDecimals(0);
        spin_box->setValue(format_settings_.at(ImageFormat::JPEG_XL).quality);
    }
}

void Window::on_start_button_clicked() {
    QStringList input_file_paths;
    for (int i = 0; i < file_list_->count(); i += 1) {
        input_file_paths.append(
            file_list_->item(i)->data(Qt::UserRole).toString()
        );
    }

    if (input_file_paths.isEmpty()) {
        log_output_->setVisible(true);
        log_output_->append("No input files selected.");
        return;
    }

    // The only point where a folder is required, so it is the only point we
    // ask. Covers the first conversion, and a folder that became unusable
    // since it was chosen.
    if (!ensure_output_dir()) {
        log_output_->setVisible(true);
        log_output_->append("No output folder selected.");
        return;
    }

    // Create unique base temp directory for this run
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch()
    )
                      .count();
    fs::path temp_base_path
        = fs::temp_directory_path() / ("comicpress_" + std::to_string(now_ms));

    try {
        fs::create_directories(temp_base_path);
        temp_base_dir_ = temp_base_path.string();
    }
    catch (const std::exception &e) {
        log_output_->setVisible(true);
        log_output_->append(
            QString("Failed to create temporary directory: %1").arg(e.what())
        );
        return;
    }

    options_.settings_group->setEnabled(false);
    start_button_->setEnabled(false);
    cancel_button_->setEnabled(true);
    log_output_->clear();
    task_queue_.clear();
    running_processes_.clear();
    running_tasks_.clear();
    jobs_.clear();
    is_processing_cancelled_ = false;
    pages_processed_ = 0;
    total_pages_ = 0;
    max_concurrent_workers_ = options_.workers_spin_box->value();

    progress_bar_->setValue(0);
    log_group_->setVisible(true);
    progress_bar_->setVisible(true);

    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto local_time = std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H-%M-%S");

    auto output_dir
        = fs::path(effective_output_dir().toStdString()) / oss.str();
    if (!create_output_dir(output_dir)) {
        log_output_->append("Failed to create output directory.");
        log_output_->setVisible(true);
        return;
    }
    output_path_ = output_dir;

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
                    = fs::path(temp_base_dir_) / source_file.stem();
                fs::create_directories(temp_archive_dir);

                auto archive = archive_read_new();
                archive_read_support_filter_all(archive);
                archive_read_support_format_all(archive);

                auto archive_open = archive_read_open_filename(
                    archive, source_file.string().c_str(), 10240
                );
                if (archive_open != ARCHIVE_OK) {
                    log_output_->append(
                        QString("Error: failed to read archive '%1'.")
                            .arg(source_file.string())
                    );
                    log_output_->setVisible(true);
                    return;
                }

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

                jobs_[file_qstr].pages_total = page_count;
                total_pages_ += page_count;
                jobs_[file_qstr].pages_processed = 0;
                if (page_count == 0) {
                    log_output_->setVisible(true);
                    log_output_->append(
                        "Archive " + file_qstr + " contains no files."
                    );
                    continue;
                }

                archive = archive_read_new();
                archive_read_support_filter_all(archive);
                archive_read_support_format_all(archive);
                archive_read_open_filename(
                    archive, source_file.string().c_str(), 10240
                );

                int i = 0;
                while (archive_read_next_header(archive, &entry)
                       == ARCHIVE_OK) {
                    if (archive_entry_filetype(entry) != AE_IFREG) {
                        continue;
                    }

                    auto task = create_task(source_file, temp_archive_dir, i);
                    task.path_in_archive = archive_entry_pathname(entry);

                    fs::path entry_path(task.path_in_archive);
                    task.output_base_name
                        = entry_path.replace_extension("").string();

                    task_queue_.enqueue(task);
                    i += 1;
                }
                archive_read_close(archive);
                archive_read_free(archive);
            }
#if defined(PDF_ENABLED)
            else if (extension == ".pdf") {
                FPDF_DOCUMENT doc
                    = FPDF_LoadDocument(source_file.string().c_str(), nullptr);
                if (!doc) {
                    log_output_->setVisible(true);
                    log_output_->append(QString(
                                            "Error: Cannot open PDF "
                                            "document %1. Error code: %2"
                    )
                                            .arg(file_qstr)
                                            .arg(FPDF_GetLastError()));
                    continue;
                }

                auto page_count = FPDF_GetPageCount(doc);
                if (page_count == 0) {
                    log_output_->setVisible(true);
                    log_output_->append(
                        "PDF " + file_qstr + " contains no pages."
                    );
                    FPDF_CloseDocument(doc);
                    continue;
                }

                auto temp_archive_dir
                    = fs::path(temp_base_dir_) / source_file.stem();
                fs::create_directories(temp_archive_dir);
                jobs_[file_qstr].pages_total = page_count;
                total_pages_ += page_count;
                jobs_[file_qstr].pages_processed = 0;

                for (int i = 0; i < page_count; i += 1) {
                    auto task = create_task(source_file, temp_archive_dir, i);
                    task_queue_.enqueue(task);
                }
                FPDF_CloseDocument(doc);
            }
#endif
        }
        catch (const std::exception &e) {
            log_output_->setVisible(true);
            log_output_->append(QString("Error discovering tasks in %1: %2")
                                    .arg(file_qstr, e.what()));
        }
    }

    if (task_queue_.isEmpty()) {
        log_output_->setVisible(true);
        log_output_->append("No pages found to process.");
        options_.settings_group->setEnabled(true);
        start_button_->setEnabled(true);
        cancel_button_->setEnabled(false);
        progress_bar_->setVisible(false);

        // Clean up base temp dir on early exit
        if (!temp_base_dir_.empty()) {
            try {
                fs::remove_all(temp_base_dir_);
            }
            catch (const std::exception &e) {
                log_output_->setVisible(true);
                log_output_->append(
                    QString("Error cleaning up temp directory: %1")
                        .arg(e.what())
                );
            }
            temp_base_dir_.clear();
        }
        return;
    }

    progress_bar_->setMaximum(total_pages_);

    // Timer
    now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()
    )
                  .count();
    overall_timer_.start_time = ms;
    overall_timer_.last_eta_time = ms;
    overall_timer_.images_since_last_eta = 0;
    elapsed_label_->setText("Elapsed: –");
    eta_label_->setText("ETA: –");
    timer_->start(1000);

    for (int i = 0; i < max_concurrent_workers_; i += 1) {
        start_next_task();
    }
}

void Window::on_cancel_button_clicked() {
    if (is_processing_cancelled_) {
        return;
    }

    is_processing_cancelled_ = true;

    task_queue_.clear();

    for (QProcess *p : running_processes_) {
        p->kill();
    }
    running_processes_.clear();
    running_tasks_.clear();

    for (auto &archive_job : jobs_) {
        auto *widget = archive_job.widget;
        progress_bars_layout_->removeWidget(widget);
        delete widget;
    }
    jobs_.clear();
    progress_bars_group_->setVisible(false);

    progress_bar_->setVisible(false);
    progress_bar_->setValue(0);

    timer_->stop();
    options_.settings_group->setEnabled(true);
    start_button_->setEnabled(true);
    cancel_button_->setEnabled(false);

    // Clean up base temp dir on cancel
    if (!temp_base_dir_.empty()) {
        auto err_code = std::error_code{};
        fs::remove_all(temp_base_dir_, err_code);
        if (err_code) {
            log_output_->append(
                QString("Warning: failed to delete temp directory '%1': %2")
                    .arg(temp_base_dir_, err_code.message())
            );
            log_output_->setVisible(true);
        }
        temp_base_dir_.clear();
    }

    // Clean up empty output dir
    auto err_code = std::error_code{};
    if (fs::is_directory(output_path_, err_code)
        && fs::is_empty(output_path_, err_code)) {
        fs::remove(output_path_, err_code);
    }
}

void Window::on_worker_finished(
    int exit_code, QProcess::ExitStatus exit_status
) {
    auto process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }

    running_processes_.removeAll(process);
    if (!running_tasks_.contains(process)) {
        process->deleteLater();
        return;
    }
    PageTask finished_task = running_tasks_.take(process);

    if (exit_status == QProcess::CrashExit || exit_code != 0) {
        log_output_->setVisible(true);
        log_output_->append(
            QString("Worker process failed or crashed. Exit code: %1")
                .arg(exit_code)
        );
    }

    handle_task_finished();

    QString source_qstr
        = QString::fromStdString(finished_task.source_file.string());

    if (jobs_.contains(source_qstr)) {
        auto &job = jobs_[source_qstr];
        job.pages_processed += 1;
        job.timer.images_since_last_eta += 1;
        auto progress_bar = job.progress_bar;
        progress_bar->setValue(job.pages_processed);

        if (job.pages_processed == job.pages_total) {
            create_archive(source_qstr);

            auto widget = job.widget;
            progress_bars_layout_->removeWidget(widget);
            delete widget;

            jobs_.remove(source_qstr);

            if (jobs_.isEmpty()) {
                progress_bars_group_->setVisible(false);
            }
        }
    }

    if (is_processing_cancelled_) {
        if (running_processes_.isEmpty()) {
            log_output_->setVisible(true);
            log_output_->append("All running tasks have been cancelled.");
        }
    }
    else {
        if (pages_processed_ == total_pages_) {
            timer_->stop();
            options_.settings_group->setEnabled(true);
            start_button_->setEnabled(true);
            cancel_button_->setEnabled(false);

            // Clean up base temp dir on success
            if (!temp_base_dir_.empty()) {
                try {
                    fs::remove_all(temp_base_dir_);
                }
                catch (const std::exception &e) {
                    log_output_->setVisible(true);
                    log_output_->append(
                        QString("Error cleaning up temp directory: %1")
                            .arg(e.what())
                    );
                }
                temp_base_dir_.clear();
            }
        }
        else {
            start_next_task();
        }
    }

    process->deleteLater();
}

void Window::on_worker_output() {
    auto process = qobject_cast<QProcess *>(sender());
    if (process) {
        // Read line by line to prevent partial messages
        while (process->canReadLine()) {
            log_output_->setVisible(true);
            log_output_->append(process->readLine().trimmed());
        }
    }
}
