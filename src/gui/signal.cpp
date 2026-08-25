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
        &QComboBox::currentTextChanged,
        this,
        &Window::on_output_format_combo_box_changed
    );
#if defined(PDF_ENABLED)
    connect(
        options_.pdf_pixel_density_combo_box,
        &QComboBox::currentTextChanged,
        this,
        &Window::on_pdf_pixel_density_combo_box_changed
    );
#endif
    connect(
        options_.double_page_spread_combo_box,
        &QComboBox::currentTextChanged,
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

void Window::on_output_format_combo_box_changed(const QString &text) {
    auto image_format_combo = options_.image_format_combo_box;
    auto *view = qobject_cast<QListView *>(image_format_combo->view());
    if (!view) {
        return;
    }

    // EPUB does not support the AVIF or JPEG XL image formats.
    auto hidden = text == "EPUB";
    auto image_format = image_format_combo->currentText();
    if (hidden && (image_format == "AVIF" || image_format == "JPEG XL")) {
        image_format_combo->setCurrentText("PNG");
    }
    view->setRowHidden(0, hidden); // AVIF
    view->setRowHidden(2, hidden); // JPEG XL
}

#if defined(PDF_ENABLED)
void Window::on_pdf_pixel_density_combo_box_changed(const QString &text) {
    auto spin = options_.pdf_pixel_density_spin_box;
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
#endif

void Window::on_double_page_spread_changed(const QString &text) {
    bool should_show = (text == "Rotate page" || text == "Rotate and split");
    options_.rotation_options_container->setVisible(should_show);
}

void Window::on_preset_option_modified() {
    if (is_programmatically_changing_values_) {
        return;
    }

    if (display_preset_.brand != "None") {
        set_display_preset("None", "");
    }
}

void Window::on_display_preset_changed() {
    auto brand = display_preset_.brand;
    auto model = display_preset_.model;
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
    if (is_checked) {
        options_.image_compression_type_combo_box->setCurrentText("Lossless");
    }
    else if (!compression_type_changed_) {
        options_.image_compression_type_combo_box->setCurrentText("Lossy");
    }
}

void Window::on_image_format_changed() {
    options_.image_format_options_container->setVisible(true);
    auto img_format = options_.image_format_combo_box->currentText();

    auto compression_min = 0;
    auto compression_max = 9;
    auto compression_effort = 0;
    QWidget *quality_label = options_.image_quality_label_original;
    auto jxl_quality_label_visible = false;
    std::string quality_type = "Quality";
    double distance = 0.0;
    auto quality = 0;
    auto compression_type_visible = false;
    auto compression_effort_visible = true;

    if (img_format == "AVIF") {
        compression_effort = avif_compression_effort_;
        quality = avif_quality_;
        compression_type_visible = true;
    }
    else if (img_format == "JPEG") {
        quality = jpeg_quality_;
        compression_effort_visible = false;
    }
    else if (img_format == "JPEG XL") {
        compression_min = 1;
        compression_effort = jpeg_xl_compression_effort_;
        quality_label = options_.image_quality_label_jpeg_xl;
        jxl_quality_label_visible = true;
        quality_type
            = options_.image_quality_label_jpeg_xl->currentText().toStdString();
        distance = jpeg_xl_distance_;
        quality = jpeg_xl_quality_;
        compression_type_visible = true;
    }
    else if (img_format == "PNG") {
        compression_effort = png_compression_effort_;
    }
    else if (img_format == "WebP") {
        compression_max = 6;
        compression_effort = webp_compression_effort_;
        quality = webp_quality_;
        compression_type_visible = true;
    }

    // Important: This should be before the others.
    on_jpeg_xl_quality_type_changed();

    options_.image_compression_spin_box->setRange(
        compression_min, compression_max
    );
    options_.image_compression_spin_box->setValue(compression_effort);
    options_.image_quality_label = quality_label;
    options_.image_quality_label_original->setVisible(
        !jxl_quality_label_visible
    );
    options_.image_quality_label_jpeg_xl->setVisible(jxl_quality_label_visible);

    if (quality_type == "Quality") {
        options_.image_quality_spin_box->setValue(quality);
    }
    else {
        options_.image_quality_spin_box->setValue(distance);
    }

    options_.image_compression_type_label->setVisible(compression_type_visible);
    options_.image_compression_type_combo_box->setVisible(
        compression_type_visible
    );
    options_.image_compression_type_tooltip->setVisible(
        compression_type_visible
    );
    options_.image_compression_label->setVisible(compression_effort_visible);
    options_.image_compression_spin_box->setVisible(compression_effort_visible);

    on_image_compression_type_changed(false);
}

void Window::on_image_compression_changed(int state) {
    auto img_format = options_.image_format_combo_box->currentText();
    if (img_format == "AVIF") {
        avif_compression_effort_ = state;
    }
    else if (img_format == "JPEG XL") {
        jpeg_xl_compression_effort_ = state;
    }
    else if (img_format == "PNG") {
        png_compression_effort_ = state;
    }
    else if (img_format == "WebP") {
        webp_compression_effort_ = state;
    }
}

void Window::on_image_compression_type_changed_explicit() {
    on_image_compression_type_changed(true);
}

void Window::on_image_compression_type_changed(bool is_explicit) {
    auto img_format = options_.image_format_combo_box->currentText();
    auto compression_type
        = options_.image_compression_type_combo_box->currentText();
    auto image_quality_visible
        = img_format != "PNG"
       && (img_format == "JPEG" || compression_type == "Lossy");
    auto jpeg_xl_quality_tooltip_visible
        = img_format == "JPEG XL" && compression_type == "Lossy";

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
    auto img_format = options_.image_format_combo_box->currentText();
    if (img_format == "AVIF") {
        avif_quality_ = static_cast<int>(value);
    }
    else if (img_format == "JPEG") {
        jpeg_quality_ = static_cast<int>(value);
    }
    else if (img_format == "JPEG XL") {
        if (options_.image_quality_label_jpeg_xl->currentText() == "Distance") {
            jpeg_xl_distance_ = value;
        }
        else {
            jpeg_xl_quality_ = static_cast<int>(value);
        }
    }
    else if (img_format == "WebP") {
        webp_quality_ = static_cast<int>(value);
    }
}

void Window::on_jpeg_xl_quality_type_changed() {
    auto quality_type = options_.image_quality_label_jpeg_xl->currentText();

    auto min = 0.0;
    auto max = 100.0;
    auto step = 1.0;
    auto decimals = 0;

    auto img_format = options_.image_format_combo_box->currentText();
    if (img_format != "JPEG XL") {
        options_.image_quality_spin_box->setRange(min, max);
        options_.image_quality_spin_box->setSingleStep(step);
        options_.image_quality_spin_box->setDecimals(decimals);
        return;
    }

    if (quality_type == "Distance") {
        max = 15.0;
        step = 0.1;
        decimals = 2;
        auto quality = jpeg_xl_distance_;
        options_.image_quality_spin_box->setValue(quality);
    }

    options_.image_quality_spin_box->setRange(min, max);
    options_.image_quality_spin_box->setSingleStep(step);
    options_.image_quality_spin_box->setDecimals(decimals);
    auto quality = jpeg_xl_quality_;
    options_.image_quality_spin_box->setValue(quality);
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
    archive_task_counts_.clear();
    total_pages_per_archive_.clear();
    pages_processed_per_archive_.clear();
    active_file_widgets_.clear();
    active_progress_bars_.clear();
    file_elapsed_labels_.clear();
    file_eta_labels_.clear();
    file_timers_.clear();
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

                archive_task_counts_[file_qstr] = page_count;
                total_pages_per_archive_[file_qstr] = page_count;
                total_pages_ += page_count;
                pages_processed_per_archive_[file_qstr] = 0;
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
                archive_task_counts_[file_qstr] = page_count;
                total_pages_per_archive_[file_qstr] = page_count;
                total_pages_ += page_count;
                pages_processed_per_archive_[file_qstr] = 0;

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
    start_time_ = ms;
    last_eta_time_ = ms;
    images_since_last_eta_ = 0;
    last_progress_value_ = 0;
    elapsed_label_->setText("Elapsed: –");
    eta_label_->setText("ETA: –");
    timer_->start(1000);

    for (int i = 0; i < max_concurrent_workers_; ++i) {
        start_next_task();
    }
}

void Window::on_cancel_button_clicked() {
    if (is_processing_cancelled_)
        return;

    is_processing_cancelled_ = true;

    task_queue_.clear();

    for (QProcess *p : running_processes_) {
        p->kill();
    }
    running_processes_.clear();
    running_tasks_.clear();

    for (QWidget *widget : active_file_widgets_.values()) {
        progress_bars_layout_->removeWidget(widget);
        delete widget;
    }
    active_file_widgets_.clear();
    active_progress_bars_.clear();
    file_elapsed_labels_.clear();
    file_eta_labels_.clear();
    file_timers_.clear();
    pages_processed_per_archive_.clear();
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

void Window::on_worker_finished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    running_processes_.removeAll(process);
    if (!running_tasks_.contains(process)) {
        process->deleteLater();
        return;
    }
    PageTask finished_task = running_tasks_.take(process);

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        log_output_->setVisible(true);
        log_output_->append(
            QString("Worker process failed or crashed. Exit code: %1")
                .arg(exitCode)
        );
    }

    handle_task_finished();

    QString source_qstr
        = QString::fromStdString(finished_task.source_file.string());

    if (pages_processed_per_archive_.contains(source_qstr)) {
        pages_processed_per_archive_[source_qstr]++;
        if (file_timers_.contains(source_qstr)) {
            file_timers_[source_qstr].images_since_last_eta++;
        }
        if (active_progress_bars_.contains(source_qstr)) {
            auto progressBar = active_progress_bars_.value(source_qstr);
            progressBar->setValue(
                pages_processed_per_archive_.value(source_qstr)
            );
        }
    }

    if (archive_task_counts_.contains(source_qstr)) {
        archive_task_counts_[source_qstr] -= 1;
        if (archive_task_counts_[source_qstr] == 0) {
            archive_task_counts_.remove(source_qstr);
            create_archive(source_qstr);

            if (active_file_widgets_.contains(source_qstr)) {
                auto widget = active_file_widgets_.take(source_qstr);
                progress_bars_layout_->removeWidget(widget);
                delete widget;
                active_progress_bars_.remove(source_qstr);
                file_elapsed_labels_.remove(source_qstr);
                file_eta_labels_.remove(source_qstr);
                file_timers_.remove(source_qstr);
                total_pages_per_archive_.remove(source_qstr);

                if (active_file_widgets_.isEmpty()) {
                    progress_bars_group_->setVisible(false);
                }
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
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (process) {
        // Read line by line to prevent partial messages
        while (process->canReadLine()) {
            log_output_->setVisible(true);
            log_output_->append(process->readLine().trimmed());
        }
    }
}
