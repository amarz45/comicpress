#include "conversion.hpp"
#include "include/display_presets.hpp"
#include "include/settings.hpp"
#include "include/window.hpp"
#include "include/window_util.hpp"

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
#include <QProgressBar>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#ifdef __linux__
#include <sys/xattr.h>
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
        &Window::on_image_compression_type_changed
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

    static constexpr auto EPUB_UNSUPPORTED
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
    for (const auto format : EPUB_UNSUPPORTED) {
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

    auto guard
        = QScopedValueRollback(is_programmatically_changing_values_, true);

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
        options_.bit_depth_combo_box->setCurrentIndex(
            options_.bit_depth_combo_box->findData(
                QVariant::fromValue(display.bit_depth)
            )
        );
        options_.convert_to_greyscale->setChecked(!display.colour);
    }
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
        // Not `on_enable_image_quantization_changed`, which would also reset
        // the compression type.
        options_.quantization_options_container->setVisible(
            options_.enable_image_quantization_check_box->isChecked()
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
    // Not `isVisible`, which is false for everything until the window is shown.
    bool parent_visible = !options_.scale_pages_container->isHidden();
    options_.scaling_options_container->setVisible(
        is_checked && parent_visible
    );
}

void Window::on_enable_image_quantization_changed(int state) {
    bool is_checked = state == Qt::Checked;
    options_.quantization_options_container->setVisible(is_checked);
    auto combo = options_.image_compression_type_combo_box;
    auto compression_type
        = is_checked ? CompressionType::LOSSLESS : CompressionType::LOSSY;
    combo->setCurrentIndex(
        combo->findData(QVariant::fromValue(compression_type))
    );
}

void Window::on_image_format_changed() {
    options_.image_format_options_container->setVisible(true);

    auto img_format = current_image_format();
    const auto &settings = format_settings_.at(img_format);
    auto is_jpeg_xl = img_format == ImageFormat::JPEG_XL;

    {
        auto guard
            = QScopedValueRollback(is_programmatically_changing_values_, true);
        update_image_quality_spin_box();
        options_.image_compression_spin_box->setRange(
            settings.compression_effort_min, settings.compression_effort_max
        );
        options_.image_compression_spin_box->setValue(
            settings.compression_effort
        );
    }

    options_.image_quality_label
        = is_jpeg_xl
            ? static_cast<QWidget *>(options_.image_quality_label_jpeg_xl)
            : options_.image_quality_label_original;
    options_.image_quality_label_original->setVisible(!is_jpeg_xl);
    options_.image_quality_label_jpeg_xl->setVisible(is_jpeg_xl);

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

    on_image_compression_type_changed();
}

void Window::on_image_compression_changed(int state) {
    if (is_programmatically_changing_values_) {
        return;
    }
    format_settings_[current_image_format()].compression_effort = state;
}

void Window::on_image_compression_type_changed() {
    auto img_format = current_image_format();
    auto compression_type
        = options_.image_compression_type_combo_box->currentData()
              .value<CompressionType>();
    auto is_lossy = compression_type == CompressionType::LOSSY;

    auto image_quality_visible = format_settings_.at(img_format).has_quality
                              && (img_format == ImageFormat::JPEG || is_lossy);
    auto jpeg_xl_quality_tooltip_visible
        = img_format == ImageFormat::JPEG_XL && is_lossy;

    options_.image_quality_label->setVisible(image_quality_visible);
    options_.image_quality_spin_box->setVisible(image_quality_visible);
    options_.image_quality_jpeg_xl_tooltip->setVisible(
        jpeg_xl_quality_tooltip_visible
    );
}

void Window::on_image_quality_changed(double value) {
    if (is_programmatically_changing_values_) {
        return;
    }
    if (image_quality_is_distance()) {
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

bool Window::image_quality_is_distance() const {
    return current_image_format() == ImageFormat::JPEG_XL
        && jpeg_xl_distance_selected();
}

void Window::on_jpeg_xl_quality_type_changed() {
    auto guard
        = QScopedValueRollback(is_programmatically_changing_values_, true);
    update_image_quality_spin_box();
}

// Changing the range and decimals can change the value, so callers must guard
// `is_programmatically_changing_values_` to keep it out of the current format’s
// settings.
void Window::update_image_quality_spin_box() {
    auto *spin_box = options_.image_quality_spin_box;
    if (image_quality_is_distance()) {
        spin_box->setRange(0.0, 15.0);
        spin_box->setSingleStep(0.1);
        spin_box->setDecimals(2);
        spin_box->setValue(jpeg_xl_distance_);
    }
    else {
        spin_box->setRange(0.0, 100.0);
        spin_box->setSingleStep(1.0);
        spin_box->setDecimals(0);
        spin_box->setValue(format_settings_.at(current_image_format()).quality);
    }
}

void Window::on_start_button_clicked() {
    auto input_files = std::vector<fs::path>{};
    for (int i = 0; i < file_list_->count(); i += 1) {
        input_files.emplace_back(
            file_list_->item(i)->data(Qt::UserRole).toString().toStdString()
        );
    }

    if (input_files.empty()) {
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

    // Before Cancel is enabled, so it can't reach the previous run.
    conversion_.reset();

    options_.settings_group->setEnabled(false);
    start_button_->setEnabled(false);
    is_converting.store(true);
    cancel_button_->setEnabled(true);
    log_output_->clear();
    jobs_.clear();
    pages_processed_ = 0;
    total_pages_ = 0;

    progress_bar_->setValue(0);
    log_group_->setVisible(true);
    progress_bar_->setVisible(true);

    QCoreApplication::processEvents();

    conversion_ = std::make_unique<Conversion>(
        ConversionRequest{
            .input_files = std::move(input_files),
            .output_dir = effective_output_dir().toStdString(),
            .output_format = options_.output_format_combo_box->currentData()
                                 .value<OutputFormat>(),
            .processing_settings = processing_settings_from_widgets(),
            .max_workers = options_.workers_spin_box->value(),
        },
        *this
    );
    conversion_->start();
}

void Window::started(int pages_total) {
    total_pages_ = pages_total;
    progress_bar_->setMaximum(total_pages_);

    // Timer
    auto now = std::chrono::system_clock::now();
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
}

bool Window::is_display_preset_active() const {
    return display_preset.brand != "None";
}

static void for_each_settings_field(
    const Options &options, bool is_display_preset_active, auto &&apply
) {
    std::apply(
        [&](const auto &...entry) { (apply(entry), ...); },
        fields_to_widgets(options, is_display_preset_active)
    );
}

void Window::set_widgets_from_settings() {
    auto guard
        = QScopedValueRollback(is_programmatically_changing_values_, true);
    for_each_settings_field(
        options_, is_display_preset_active(), [](const auto &entry) {
            set_widget_value_from_settings(entry.field, entry.widget);
        }
    );
    restore_format_settings();

    QSettings settings;
    if (settings.contains(DISPLAY_PRESET_BRAND_SETTINGS_KEY)
        && settings.contains(DISPLAY_PRESET_MODEL_SETTINGS_KEY)) {
        auto brand = settings.value(DISPLAY_PRESET_BRAND_SETTINGS_KEY);
        auto model = settings.value(DISPLAY_PRESET_MODEL_SETTINGS_KEY);

        const auto brand_str = brand.toString().toStdString();
        const auto model_str = model.toString().toStdString();

        const auto brand_exists
            = [&] { return DISPLAY_PRESETS.contains(brand_str); };
        const auto model_exists = [&] {
            const auto &model_map = DISPLAY_PRESETS.at(brand_str);
            if (!model_map) {
                return false;
            }
            const auto it
                = std::ranges::find_if(*model_map, [&](const auto &pair) {
                      return pair.first == model_str;
                  });
            return it != model_map->end();
        };
        if (brand_exists() && model_exists()) {
            set_display_preset(brand_str, model_str);
        }
    }
}

void Window::save_settings_from_widgets() {
    QSettings settings;
    for_each_settings_field(
        options_, is_display_preset_active(), [&](const auto &entry) {
            auto settings_value = settings_value_from_widget(entry.widget);
            if (settings_value.isValid()) {
                settings.setValue(entry.field, settings_value);
            }
            else {
                // Don’t leave a stale value from when it wasn’t derived.
                settings.remove(entry.field);
            }
        }
    );

    if (is_display_preset_active()) {
        settings.setValue(
            DISPLAY_PRESET_BRAND_SETTINGS_KEY,
            QString::fromStdString(display_preset.brand)
        );
        settings.setValue(
            DISPLAY_PRESET_MODEL_SETTINGS_KEY,
            QString::fromStdString(display_preset.model)
        );
    }
    else {
        settings.remove(DISPLAY_PRESET_BRAND_SETTINGS_KEY);
        settings.remove(DISPLAY_PRESET_MODEL_SETTINGS_KEY);
    }
    persist_format_settings();
}

// Calls `apply` with each image format’s settings key prefix, the format, and
// its entry in `format_settings`.
static void for_each_format_settings(
    const QComboBox *image_format_combo_box, auto &format_settings, auto &&apply
) {
    for (auto i = 0; i < image_format_combo_box->count(); i += 1) {
        auto format = image_format_combo_box->itemData(i).value<ImageFormat>();
        auto key
            = image_format_combo_box->itemData(i, SETTINGS_KEY_ROLE).toString();
        auto prefix = QString(IMAGE_FORMATS_SETTINGS_GROUP) + '/' + key + '/';
        apply(prefix, format, format_settings.at(format));
    }
}

void Window::persist_format_settings() {
    QSettings settings;
    for_each_format_settings(
        options_.image_format_combo_box,
        format_settings_,
        [&](const QString &prefix,
            ImageFormat format,
            const FormatSettings &format_settings) {
            if (format_settings.has_quality) {
                settings.setValue(
                    prefix + FORMAT_QUALITY_SETTINGS_KEY,
                    format_settings.quality
                );
            }
            if (format_settings.has_compression_effort) {
                settings.setValue(
                    prefix + FORMAT_COMPRESSION_EFFORT_SETTINGS_KEY,
                    format_settings.compression_effort
                );
            }
            if (format == ImageFormat::JPEG_XL) {
                settings.setValue(
                    prefix + FORMAT_DISTANCE_SETTINGS_KEY, jpeg_xl_distance_
                );
            }
        }
    );
}

// Out-of-range values aren’t clamped here, since the spin boxes clamp
// everything they’re given.
void Window::restore_format_settings() {
    for_each_format_settings(
        options_.image_format_combo_box,
        format_settings_,
        [&](const QString &prefix,
            ImageFormat format,
            FormatSettings &format_settings) {
            with_setting<int>(
                prefix + FORMAT_QUALITY_SETTINGS_KEY,
                [&](int quality) { format_settings.quality = quality; }
            );
            with_setting<int>(
                prefix + FORMAT_COMPRESSION_EFFORT_SETTINGS_KEY,
                [&](int effort) { format_settings.compression_effort = effort; }
            );
            if (format == ImageFormat::JPEG_XL) {
                with_setting<double>(
                    prefix + FORMAT_DISTANCE_SETTINGS_KEY,
                    [&](double distance) { jpeg_xl_distance_ = distance; }
                );
            }
        }
    );
}

void Window::on_cancel_button_clicked() {
    if (!conversion_) {
        return;
    }

    conversion_->cancel();
    conversion_.reset();

    for (auto &archive_job : jobs_) {
        auto *widget = archive_job.widget;
        progress_bars_layout_->removeWidget(widget);
        delete widget;
    }
    jobs_.clear();
    progress_bars_group_->setVisible(false);

    progress_bar_->setVisible(false);
    progress_bar_->setValue(0);

    log_output_->setVisible(true);
    log_output_->append("All running tasks have been cancelled.");

    finish_run();
}

void Window::file_finished(const fs::path &source) {
    auto widget = jobs_.take(QString::fromStdString(source.string())).widget;
    progress_bars_layout_->removeWidget(widget);
    delete widget;

    if (jobs_.isEmpty()) {
        progress_bars_group_->setVisible(false);
    }
}

void Window::finished(bool) {
    // Nothing was found to process, so there is no progress to show.
    if (total_pages_ == 0) {
        progress_bar_->setVisible(false);
    }
    finish_run();
}

// Every exit from a run goes through here, so a failure can’t leave the
// button disabled.
void Window::finish_run() {
    is_converting.store(false);
    timer_->stop();
    options_.settings_group->setEnabled(true);
    start_button_->setEnabled(true);
    cancel_button_->setEnabled(false);
}
