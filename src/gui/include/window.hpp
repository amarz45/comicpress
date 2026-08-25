#pragma once

#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <cstdint>
#include <deque>
#include <optional>
#include <qtconfigmacros.h>
#include <string>
#include <utility>

#include "../../include/task.hpp"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QMainWindow;
class QProcess;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;
class QTabWidget;
class QTextEdit;
class QTimer;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

template <typename T>
class BoundedDeque {
    size_t max_size_;

  public:
    std::deque<T> dq;
    BoundedDeque(size_t n) : max_size_(n) {
    }

    void push_back(const T &val) {
        if (dq.size() == max_size_) {
            dq.pop_front();
        }
        dq.push_back(val);
    }

    void push_front(const T &val) {
        if (dq.size() == max_size_) {
            dq.pop_back();
        }
        dq.push_front(val);
    }
};

struct Options {
    QGroupBox *settings_group;
    QFormLayout *settings_layout;
    QCheckBox *advanced_options_check_box;
#if defined(PDF_ENABLED)
    QLabel *pdf_pixel_density_label;
    QLabel *pdf_pixel_density_tooltip;
    QComboBox *pdf_pixel_density_combo_box;
    QWidget *pdf_options_container;
    QSpinBox *pdf_pixel_density_spin_box;
#endif
    QCheckBox *convert_to_greyscale;
    QComboBox *double_page_spread_combo_box;
    QLabel *linear_light_resampling_label;
    QCheckBox *linear_light_resampling_check_box;
    QWidget *linear_light_resampling_container;
    QCheckBox *remove_spine_check_box;
    QCheckBox *contrast_check_box;
    QPushButton *display_preset_button;
    QComboBox *output_format_combo_box;
    QLabel *scale_pages_label;
    QWidget *scale_pages_container;
    QCheckBox *enable_image_scaling_check_box;
    QWidget *scaling_options_container;
    QSpinBox *width_spin_box;
    QSpinBox *height_spin_box;
    QComboBox *resampler_combo_box;
    QCheckBox *enable_image_quantization_check_box;
    QLabel *quantize_pages_label;
    QWidget *quantize_pages_container;
    QWidget *quantization_options_container;
    QComboBox *bit_depth_combo_box;
    QDoubleSpinBox *dithering_spin_box;
    QLabel *image_format_label;
    QWidget *image_format_container;
    QWidget *image_format_options_container;
    QComboBox *image_format_combo_box;
    QLabel *image_compression_label;
    QSpinBox *image_compression_spin_box;
    QComboBox *image_compression_type_combo_box;
    QLabel *image_compression_type_label;
    QLabel *image_compression_type_tooltip;
    QLabel *image_quality_jpeg_xl_tooltip;
    QDoubleSpinBox *image_quality_spin_box;
    QWidget *image_quality_label;
    QLabel *image_quality_label_original;
    QComboBox *image_quality_label_jpeg_xl;
    QLabel *workers_label;
    QSpinBox *workers_spin_box;
    QWidget *rotation_options_container;
    QComboBox *rotation_direction_combo_box;
};

struct FileTimer {
    std::optional<int64_t> start_time;
    std::optional<int64_t> last_eta_time;
    int images_since_last_eta = 0;
    BoundedDeque<std::pair<int64_t, int64_t>> eta_samples;

    FileTimer() : eta_samples(5) {
    }
};

struct DisplayPreset {
    std::string brand;
    std::string model;
};

class Window : public QMainWindow {
    Q_OBJECT

  public:
    DisplayPreset display_preset = DisplayPreset{
        .brand = "Custom",
        .model = "",
    };
    explicit Window(QWidget *parent = nullptr);
    ~Window() override;

  private slots:
    void on_start_button_clicked();
    void on_cancel_button_clicked();
    void handle_log_message(const QString &message);
    void handle_task_finished();
    void start_next_task();
    void on_worker_finished(int exit_code, QProcess::ExitStatus exit_status);
    void on_worker_output();
    void on_add_files_clicked();
    void on_remove_selected_clicked();
    void on_clear_all_clicked();
    void on_browse_output_clicked();
    // Folder to actually write to, which may differ from what output_dir_field
    // shows.
    QString effective_output_dir() const;
    void on_output_format_combo_box_changed(const QString &text);
#if defined(PDF_ENABLED)
    void on_pdf_pixel_density_combo_box_changed(const QString &text);
#endif
    void on_double_page_spread_changed(const QString &text);
    void on_display_preset_changed();
    void on_preset_option_modified();
    void on_advanced_options_changed(int state);
    void on_enable_image_scaling_changed(int state);
    void on_enable_image_quantization_changed(int state);
    void on_image_format_changed();
    void on_image_compression_changed(int state);
    void on_image_compression_type_changed(bool is_explicit);
    void on_image_compression_type_changed_explicit();
    void on_image_quality_changed(double value);
    void on_jpeg_xl_quality_type_changed();

  private:
    QWidget *central_widget_;
    QVBoxLayout *main_layout_;

    // Timer
    QTimer *timer_;
    std::optional<int64_t> start_time_;
    std::optional<int64_t> last_eta_time_;
    int images_since_last_eta_;
    BoundedDeque<std::pair<int64_t, int64_t>> eta_samples_;

    // Input and output
    QListWidget *file_list_;
    QPushButton *add_files_button_;
    QPushButton *remove_selected_button_;
    QPushButton *clear_all_button_;
    QLineEdit *output_dir_field_;
    // Writable path for the chosen output folder (a portal path under
    // Flatpak); empty until the user picks one, while output_dir_field shows
    // its host path.
    QString output_dir_io_path_;
    fs::path output_path_;
    QPushButton *browse_output_button_;

    void set_output_dir(const QString &io_path);
    void persist_output_dir();
    void restore_output_dir();
    bool ensure_output_dir();
    QGroupBox *progress_bars_group_;
    QVBoxLayout *progress_bars_layout_;

    Options options_;

    // Progress
    QLabel *elapsed_label_;
    QLabel *eta_label_;
    QProgressBar *progress_bar_;
    QTextEdit *log_output_;
    QPushButton *start_button_;
    QPushButton *cancel_button_;

    // Process Management
    QQueue<PageTask> task_queue_;
    QList<QProcess *> running_processes_;
    QMap<QProcess *, PageTask> running_tasks_;
    QMap<QString, int> archive_task_counts_;
    QMap<QString, int> total_pages_per_archive_;
    QMap<QString, int> pages_processed_per_archive_;
    QMap<QString, QWidget *> active_file_widgets_;
    QMap<QString, QProgressBar *> active_progress_bars_;
    QMap<QString, QLabel *> file_elapsed_labels_;
    QMap<QString, QLabel *> file_eta_labels_;
    QMap<QString, FileTimer> file_timers_;
    int max_concurrent_workers_;
    bool is_processing_cancelled_;
    bool is_programmatically_changing_values_;

    // Timer
    void update_time_labels();
    void update_overall_time_labels();
    void update_file_time_labels(const QString &file);

    // UI setup
    void setup_ui();
    QGroupBox *create_io_group();
    QGroupBox *create_settings_group();
    void create_log_group();
    QGroupBox *log_group_;

    void add_display_presets_widget();

    // Helper methods
    PageTask
    create_task(const fs::path &source_file, fs::path output_dir, int page_num);
    void update_file_list_buttons();
    void connect_signals();
    void set_display_preset(const std::string &brand, const std::string &model);
    void create_archive(const QString &source_archive_path);

    int total_pages_;
    int pages_processed_;

    int avif_compression_effort_ = 4;
    int avif_quality_ = 50;
    int jpeg_quality_ = 80;
    int jpeg_xl_compression_effort_ = 7;
    double jpeg_xl_distance_ = 1.0;
    int jpeg_xl_quality_ = 75;
    int png_compression_effort_ = 6;
    int webp_compression_effort_ = 4;
    int webp_quality_ = 80;
    bool compression_type_changed_ = false;

    std::string temp_base_dir_;
};
